#include <cstdio>
#include <cstdlib>
#include <cstdint>

extern "C" {
#include<libavcodec/avcodec.h>

#include "libavformat/avformat.h"
#include "libavformat/avio.h"

#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libavutil/audio_fifo.h>
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"

#include "libswresample/swresample.h"
}

#define OUTPUT_BIT_RATE 96000
/* The number of output channels */
#define OUTPUT_CHANNELS 2

#ifdef av_err2str
#undef av_err2str
#include <string>
av_always_inline std::string av_err2string(int errnum) {
    char str[AV_ERROR_MAX_STRING_SIZE];
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}
#define av_err2str(err) av_err2string(err).c_str()
#endif  // av_err2str


static int open_input_file(const char* filename,
    AVFormatContext** input_format_context,
    AVCodecContext** input_codec_context)
{
    AVCodecContext* avctx;
    const AVCodec* input_codec;
    const AVStream* stream;
    int error;

    /* Open the input file to read from it. */
    if ((error = avformat_open_input(input_format_context, filename, NULL,
        NULL)) < 0) {
        fprintf(stderr, "Could not open input file '%s' (error '%s')\n",
            filename, av_err2str(error));
        *input_format_context = NULL;
        return error;
    }

    /* Get information on the input file (number of streams etc.). */
    if ((error = avformat_find_stream_info(*input_format_context, NULL)) < 0) {
        fprintf(stderr, "Could not open find stream info (error '%s')\n",
            av_err2str(error));
        avformat_close_input(input_format_context);
        return error;
    }

    /* Make sure that there is only one stream in the input file. */
    if ((*input_format_context)->nb_streams != 1) {
        fprintf(stderr, "Expected one audio input stream, but found %d\n",
            (*input_format_context)->nb_streams);
        avformat_close_input(input_format_context);
        return AVERROR_EXIT;
    }

    stream = (*input_format_context)->streams[0];

    /* Find a decoder for the audio stream. */
    if (!(input_codec = avcodec_find_decoder(stream->codecpar->codec_id))) {
        fprintf(stderr, "Could not find input codec\n");
        avformat_close_input(input_format_context);
        return AVERROR_EXIT;
    }

    /* Allocate a new decoding context. */
    avctx = avcodec_alloc_context3(input_codec);
    if (!avctx) {
        fprintf(stderr, "Could not allocate a decoding context\n");
        avformat_close_input(input_format_context);
        return AVERROR(ENOMEM);
    }

    /* Initialize the stream parameters with demuxer information. */
    error = avcodec_parameters_to_context(avctx, stream->codecpar);
    if (error < 0) {
        avformat_close_input(input_format_context);
        avcodec_free_context(&avctx);
        return error;
    }

    /* Open the decoder for the audio stream to use it later. */
    if ((error = avcodec_open2(avctx, input_codec, NULL)) < 0) {
        fprintf(stderr, "Could not open input codec (error '%s')\n",
            av_err2str(error));
        avcodec_free_context(&avctx);
        avformat_close_input(input_format_context);
        return error;
    }

    /* Set the packet timebase for the decoder. */
    avctx->pkt_timebase = stream->time_base;

    /* Save the decoder context for easier access later. */
    *input_codec_context = avctx;

    return 0;
}


static int open_output_file(const char* filename,
    AVCodecContext* input_codec_context,
    AVFormatContext** output_format_context,
    AVCodecContext** output_codec_context)
{
    AVCodecContext* avctx = NULL;
    AVIOContext* output_io_context = NULL;
    AVStream* stream = NULL;
    const AVCodec* output_codec = NULL;
    int error;

    /* Open the output file to write to it. */
    if ((error = avio_open(&output_io_context, filename,
        AVIO_FLAG_WRITE)) < 0) {
        fprintf(stderr, "Could not open output file '%s' (error '%s')\n",
            filename, av_err2str(error));
        return error;
    }

    /* Create a new format context for the output container format. */
    if (!(*output_format_context = avformat_alloc_context())) {
        fprintf(stderr, "Could not allocate output format context\n");
        return AVERROR(ENOMEM);
    }

    /* Associate the output file (pointer) with the container format context. */
    (*output_format_context)->pb = output_io_context;

    /* Guess the desired container format based on the file extension. */
    if (!((*output_format_context)->oformat = av_guess_format(NULL, filename,
        NULL))) {
        fprintf(stderr, "Could not find output file format\n");
        goto cleanup;
    }

    if (!((*output_format_context)->url = av_strdup(filename))) {
        fprintf(stderr, "Could not allocate url.\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
    }

    /* Find the encoder to be used by its name. */
    if (!(output_codec = avcodec_find_encoder(AV_CODEC_ID_AAC))) {
        fprintf(stderr, "Could not find an AAC encoder.\n");
        goto cleanup;
    }

    /* Create a new audio stream in the output file container. */
    if (!(stream = avformat_new_stream(*output_format_context, NULL))) {
        fprintf(stderr, "Could not create new stream\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
    }

    avctx = avcodec_alloc_context3(output_codec);
    if (!avctx) {
        fprintf(stderr, "Could not allocate an encoding context\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
    }

    /* Set the basic encoder parameters.
     * The input file's sample rate is used to avoid a sample rate conversion. */
    av_channel_layout_default(&avctx->ch_layout, OUTPUT_CHANNELS);
    avctx->sample_rate = input_codec_context->sample_rate;
    avctx->sample_fmt = output_codec->sample_fmts[0];
    avctx->bit_rate = OUTPUT_BIT_RATE;

    /* Set the sample rate for the container. */
    stream->time_base.den = input_codec_context->sample_rate;
    stream->time_base.num = 1;

    /* Some container formats (like MP4) require global headers to be present.
     * Mark the encoder so that it behaves accordingly. */
    if ((*output_format_context)->oformat->flags & AVFMT_GLOBALHEADER)
        avctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    /* Open the encoder for the audio stream to use it later. */
    if ((error = avcodec_open2(avctx, output_codec, NULL)) < 0) {
        fprintf(stderr, "Could not open output codec (error '%s')\n",
            av_err2str(error));
        goto cleanup;
    }

    error = avcodec_parameters_from_context(stream->codecpar, avctx);
    if (error < 0) {
        fprintf(stderr, "Could not initialize stream parameters\n");
        goto cleanup;
    }

    /* Save the encoder context for easier access later. */
    *output_codec_context = avctx;

    return 0;

cleanup:
    avcodec_free_context(&avctx);
    avio_closep(&(*output_format_context)->pb);
    avformat_free_context(*output_format_context);
    *output_format_context = NULL;
    return error < 0 ? error : AVERROR_EXIT;
}


static int init_packet(AVPacket** packet)
{
    if (!(*packet = av_packet_alloc())) {
        fprintf(stderr, "Could not allocate packet\n");
        return AVERROR(ENOMEM);
    }
    return 0;
}


static int init_input_frame(AVFrame** frame)
{
    if (!(*frame = av_frame_alloc())) {
        fprintf(stderr, "Could not allocate input frame\n");
        return AVERROR(ENOMEM);
    }
    return 0;
}


static int init_resampler(AVCodecContext* input_codec_context,
    AVCodecContext* output_codec_context,
    SwrContext** resample_context)
{
    int error;

    /*
     * Create a resampler context for the conversion.
     * Set the conversion parameters.
     */
    error = swr_alloc_set_opts2(resample_context,
        &output_codec_context->ch_layout,
        output_codec_context->sample_fmt,
        output_codec_context->sample_rate,
        &input_codec_context->ch_layout,
        input_codec_context->sample_fmt,
        input_codec_context->sample_rate,
        0, NULL);
    if (error < 0) {
        fprintf(stderr, "Could not allocate resample context\n");
        return error;
    }

    av_assert0(output_codec_context->sample_rate == input_codec_context->sample_rate);

    /* Open the resampler with the specified parameters. */
    if ((error = swr_init(*resample_context)) < 0) {
        fprintf(stderr, "Could not open resample context\n");
        swr_free(resample_context);
        return error;
    }
    return 0;
}


static int init_fifo(AVAudioFifo** fifo, AVCodecContext* output_codec_context)
{
    /* Create the FIFO buffer based on the specified output sample format. */
    if (!(*fifo = av_audio_fifo_alloc(output_codec_context->sample_fmt,
        output_codec_context->ch_layout.nb_channels, 1))) {
        fprintf(stderr, "Could not allocate FIFO\n");
        return AVERROR(ENOMEM);
    }
    return 0;
}

static int write_output_file_header(AVFormatContext* output_format_context)
{
    int error;
    if ((error = avformat_write_header(output_format_context, NULL)) < 0) {
        fprintf(stderr, "Could not write output file header (error '%s')\n",
            av_err2str(error));
        return error;
    }
    return 0;
}

static int decode_audio_frame(AVFrame* frame,
    AVFormatContext* input_format_context,
    AVCodecContext* input_codec_context,
    int* data_present, int* finished)
{
    /* Packet used for temporary storage. */
    AVPacket* input_packet;
    int error;

    error = init_packet(&input_packet);
    if (error < 0)
        return error;

    *data_present = 0;
    *finished = 0;
    /* Read one audio frame from the input file into a temporary packet. */
    if ((error = av_read_frame(input_format_context, input_packet)) < 0) {
        /* If we are at the end of the file, flush the decoder below. */
        if (error == AVERROR_EOF)
            *finished = 1;
        else {
            fprintf(stderr, "Could not read frame (error '%s')\n",
                av_err2str(error));
            goto cleanup;
        }
    }

    /* Send the audio frame stored in the temporary packet to the decoder.
     * The input audio stream decoder is used to do this. */
    if ((error = avcodec_send_packet(input_codec_context, input_packet)) < 0) {
        fprintf(stderr, "Could not send packet for decoding (error '%s')\n",
            av_err2str(error));
        goto cleanup;
    }

    /* Receive one frame from the decoder. */
    error = avcodec_receive_frame(input_codec_context, frame);
    /* If the decoder asks for more data to be able to decode a frame,
     * return indicating that no data is present. */
    if (error == AVERROR(EAGAIN)) {
        error = 0;
        goto cleanup;
        /* If the end of the input file is reached, stop decoding. */
    }
    else if (error == AVERROR_EOF) {
        *finished = 1;
        error = 0;
        goto cleanup;
    }
    else if (error < 0) {
        fprintf(stderr, "Could not decode frame (error '%s')\n",
            av_err2str(error));
        goto cleanup;
        /* Default case: Return decoded data. */
    }
    else {
        *data_present = 1;
        goto cleanup;
    }

cleanup:
    av_packet_free(&input_packet);
    return error;
}


static int init_converted_samples(uint8_t*** converted_input_samples,
    AVCodecContext* output_codec_context,
    int frame_size)
{
    int error;

    /* Allocate as many pointers as there are audio channels.
     * Each pointer will later point to the audio samples of the corresponding
     * channels (although it may be NULL for interleaved formats).
     */
    if (!(*converted_input_samples = static_cast<uint8_t**>(calloc(output_codec_context->ch_layout.nb_channels,
        sizeof(**converted_input_samples))))) {
        fprintf(stderr, "Could not allocate converted input sample pointers\n");
        return AVERROR(ENOMEM);
    }

    if ((error = av_samples_alloc(*converted_input_samples, NULL,
        output_codec_context->ch_layout.nb_channels,
        frame_size,
        output_codec_context->sample_fmt, 0)) < 0) {
        fprintf(stderr,
            "Could not allocate converted input samples (error '%s')\n",
            av_err2str(error));
        av_freep(&(*converted_input_samples)[0]);
        free(*converted_input_samples);
        return error;
    }
    return 0;
}


static int convert_samples(const uint8_t** input_data,
    uint8_t** converted_data, const int frame_size,
    SwrContext* resample_context)
{
    int error;

    /* Convert the samples using the resampler. */
    if ((error = swr_convert(resample_context,
        converted_data, frame_size,
        input_data, frame_size)) < 0) {
        fprintf(stderr, "Could not convert input samples (error '%s')\n",
            av_err2str(error));
        return error;
    }

    return 0;
}


static int add_samples_to_fifo(AVAudioFifo* fifo,
    uint8_t** converted_input_samples,
    const int frame_size)
{
    int error;

    /* Make the FIFO as large as it needs to be to hold both,
     * the old and the new samples. */
    if ((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frame_size)) < 0) {
        fprintf(stderr, "Could not reallocate FIFO\n");
        return error;
    }

    /* Store the new samples in the FIFO buffer. */
    if (av_audio_fifo_write(fifo, (void**)converted_input_samples,
        frame_size) < frame_size) {
        fprintf(stderr, "Could not write data to FIFO\n");
        return AVERROR_EXIT;
    }
    return 0;
}


static int read_decode_convert_and_store(AVAudioFifo* fifo,
    AVFormatContext* input_format_context,
    AVCodecContext* input_codec_context,
    AVCodecContext* output_codec_context,
    SwrContext* resampler_context,
    int* finished)
{
    /* Temporary storage of the input samples of the frame read from the file. */
    AVFrame* input_frame = NULL;
    /* Temporary storage for the converted input samples. */
    uint8_t** converted_input_samples = NULL;
    int data_present;
    int ret = AVERROR_EXIT;

    /* Initialize temporary storage for one input frame. */
    if (init_input_frame(&input_frame))
        goto cleanup;
    /* Decode one frame worth of audio samples. */
    if (decode_audio_frame(input_frame, input_format_context,
        input_codec_context, &data_present, finished))
        goto cleanup;
    /* If we are at the end of the file and there are no more samples
     * in the decoder which are delayed, we are actually finished.
     * This must not be treated as an error. */
    if (*finished) {
        ret = 0;
        goto cleanup;
    }
    /* If there is decoded data, convert and store it. */
    if (data_present) {
        /* Initialize the temporary storage for the converted input samples. */
        if (init_converted_samples(&converted_input_samples, output_codec_context,
            input_frame->nb_samples))
            goto cleanup;

        /* Convert the input samples to the desired output sample format.
         * This requires a temporary storage provided by converted_input_samples. */
        if (convert_samples((const uint8_t**)input_frame->extended_data, converted_input_samples,
            input_frame->nb_samples, resampler_context))
            goto cleanup;

        /* Add the converted input samples to the FIFO buffer for later processing. */
        if (add_samples_to_fifo(fifo, converted_input_samples,
            input_frame->nb_samples))
            goto cleanup;
        ret = 0;
    }
    ret = 0;

cleanup:
    if (converted_input_samples) {
        av_freep(&converted_input_samples[0]);
        free(converted_input_samples);
    }
    av_frame_free(&input_frame);

    return ret;
}


static int init_output_frame(AVFrame** frame,
    AVCodecContext* output_codec_context,
    int frame_size)
{
    int error;

    /* Create a new frame to store the audio samples. */
    if (!(*frame = av_frame_alloc())) {
        fprintf(stderr, "Could not allocate output frame\n");
        return AVERROR_EXIT;
    }

    (*frame)->nb_samples = frame_size;
    av_channel_layout_copy(&(*frame)->ch_layout, &output_codec_context->ch_layout);
    (*frame)->format = output_codec_context->sample_fmt;
    (*frame)->sample_rate = output_codec_context->sample_rate;

    /* Allocate the samples of the created frame. This call will make
     * sure that the audio frame can hold as many samples as specified. */
    if ((error = av_frame_get_buffer(*frame, 0)) < 0) {
        fprintf(stderr, "Could not allocate output frame samples (error '%s')\n",
            av_err2str(error));
        av_frame_free(frame);
        return error;
    }

    return 0;
}


static int64_t pts = 0;


static int encode_audio_frame(AVFrame* frame,
    AVFormatContext* output_format_context,
    AVCodecContext* output_codec_context,
    int* data_present)
{
    /* Packet used for temporary storage. */
    AVPacket* output_packet;
    int error;

    error = init_packet(&output_packet);
    if (error < 0)
        return error;

    /* Set a timestamp based on the sample rate for the container. */
    if (frame) {
        frame->pts = pts;
        pts += frame->nb_samples;
    }

    *data_present = 0;
    /* Send the audio frame stored in the temporary packet to the encoder.
     * The output audio stream encoder is used to do this. */
    error = avcodec_send_frame(output_codec_context, frame);
    /* Check for errors, but proceed with fetching encoded samples if the
     *  encoder signals that it has nothing more to encode. */
    if (error < 0 && error != AVERROR_EOF) {
        fprintf(stderr, "Could not send packet for encoding (error '%s')\n",
            av_err2str(error));
        goto cleanup;
    }

    /* Receive one encoded frame from the encoder. */
    error = avcodec_receive_packet(output_codec_context, output_packet);
    /* If the encoder asks for more data to be able to provide an
     * encoded frame, return indicating that no data is present. */
    if (error == AVERROR(EAGAIN)) {
        error = 0;
        goto cleanup;
        /* If the last frame has been encoded, stop encoding. */
    }
    else if (error == AVERROR_EOF) {
        error = 0;
        goto cleanup;
    }
    else if (error < 0) {
        fprintf(stderr, "Could not encode frame (error '%s')\n",
            av_err2str(error));
        goto cleanup;
        /* Default case: Return encoded data. */
    }
    else {
        *data_present = 1;
    }

    if (*data_present &&
        (error = av_write_frame(output_format_context, output_packet)) < 0) {
        fprintf(stderr, "Could not write frame (error '%s')\n",
            av_err2str(error));
        goto cleanup;
    }

cleanup:
    av_packet_free(&output_packet);
    return error;
}


static int load_encode_and_write(AVAudioFifo* fifo,
    AVFormatContext* output_format_context,
    AVCodecContext* output_codec_context)
{

    AVFrame* output_frame;

    const int frame_size = FFMIN(av_audio_fifo_size(fifo),
        output_codec_context->frame_size);
    int data_written;

    if (init_output_frame(&output_frame, output_codec_context, frame_size))
        return AVERROR_EXIT;

    if (av_audio_fifo_read(fifo, (void**)output_frame->data, frame_size) < frame_size) {
        fprintf(stderr, "Could not read data from FIFO\n");
        av_frame_free(&output_frame);
        return AVERROR_EXIT;
    }

    /* Encode one frame worth of audio samples. */
    if (encode_audio_frame(output_frame, output_format_context,
        output_codec_context, &data_written)) {
        av_frame_free(&output_frame);
        return AVERROR_EXIT;
    }
    av_frame_free(&output_frame);
    return 0;
}


static int write_output_file_trailer(AVFormatContext* output_format_context)
{
    int error;
    if ((error = av_write_trailer(output_format_context)) < 0) {
        fprintf(stderr, "Could not write output file trailer (error '%s')\n",
            av_err2str(error));
        return error;
    }
    return 0;
}

int main(int argc, char** argv)
{
    AVFormatContext* input_format_context = NULL, * output_format_context = NULL;
    AVCodecContext* input_codec_context = NULL, * output_codec_context = NULL;
    SwrContext* resample_context = NULL;
    AVAudioFifo* fifo = NULL;
    int ret = AVERROR_EXIT;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
        exit(1);
    }

    /* Open the input file for reading. */
    if (open_input_file(argv[1], &input_format_context,
        &input_codec_context))
        goto cleanup;
    /* Open the output file for writing. */
    if (open_output_file(argv[2], input_codec_context,
        &output_format_context, &output_codec_context))
        goto cleanup;
    /* Initialize the resampler to be able to convert audio sample formats. */
    if (init_resampler(input_codec_context, output_codec_context,
        &resample_context))
        goto cleanup;
    /* Initialize the FIFO buffer to store audio samples to be encoded. */
    if (init_fifo(&fifo, output_codec_context))
        goto cleanup;
    /* Write the header of the output file container. */
    if (write_output_file_header(output_format_context))
        goto cleanup;

    while (1) {

        const int output_frame_size = output_codec_context->frame_size;
        int finished = 0;

 
        while (av_audio_fifo_size(fifo) < output_frame_size) {
  
            if (read_decode_convert_and_store(fifo, input_format_context,
                input_codec_context,
                output_codec_context,
                resample_context, &finished))
                goto cleanup;

            if (finished)
                break;
        }

        while (av_audio_fifo_size(fifo) >= output_frame_size ||
            (finished && av_audio_fifo_size(fifo) > 0))

            if (load_encode_and_write(fifo, output_format_context,
                output_codec_context))
                goto cleanup;

        if (finished) {
            int data_written;

            do {
                if (encode_audio_frame(NULL, output_format_context,
                    output_codec_context, &data_written))
                    goto cleanup;
            } while (data_written);
            break;
        }
    }

    /* Write the trailer of the output file container. */
    if (write_output_file_trailer(output_format_context))
        goto cleanup;
    ret = 0;

cleanup:
    if (fifo)
        av_audio_fifo_free(fifo);
    swr_free(&resample_context);
    if (output_codec_context)
        avcodec_free_context(&output_codec_context);
    if (output_format_context) {
        avio_closep(&output_format_context->pb);
        avformat_free_context(output_format_context);
    }
    if (input_codec_context)
        avcodec_free_context(&input_codec_context);
    if (input_format_context)
        avformat_close_input(&input_format_context);

    return ret;
}