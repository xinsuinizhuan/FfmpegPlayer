# Qt-Ffmpeg

-   [Simplified Chinese](README.md)
-   [English](README.en.md)

## QFfmpegPlayer

<div align=center><img src="doc/player.jpeg"></div>

### Requires a powerful opengl and vulkan yuv rendering module

1.  Opengl's fragment shader currently supports limited image formats;
2.  In WidgetRender, use the QImage::Format_RGB32 and QImage::Format_ARGB32_Premultiplied image formats whenever possible. The following reasons:
    1.  Avoid most rendering directly to most of these formats using QPainter. Rendering is best optimized to the Format_RGB32  and Format_ARGB32_Premultiplied formats, and secondarily for rendering to the Format_RGB16, Format_RGBX8888,  Format_RGBA8888_Premultiplied, Format_RGBX64 and Format_RGBA64_Premultiplied formats.

### How to adjust image based on AVColorTransferCharacteristic?

#### 1. How to modify the shader when rendering with opengl?

#### 2. In the case of non-opengl rendering, how to add a filter to achieve image compensation?

1.  reference[MPV video_shaders](https://github.com/mpv-player/mpv/blob/master/video/out/gpu/video_shaders.c#L341), the effect is not very good; there should be something missing.

```cpp
void pass_linearize(struct gl_shader_cache *sc, enum mp_csp_trc trc);
void pass_delinearize(struct gl_shader_cache *sc, enum mp_csp_trc trc);
```

2.  On the NV12 shader, refer to the MPV implementation of the SMPTE2084 image adjustment shader.

```glsl
#version 330 core

in vec2 TexCord;    // 纹理坐标
out vec4 FragColor; // 输出颜色

uniform sampler2D tex_y;
uniform sampler2D tex_uv;

uniform vec3 offset;
uniform mat3 colorConversion;

const float PQ_M1 = 2610. / 4096 * 1. / 4, PQ_M2 = 2523. / 4096 * 128, PQ_C1 = 3424. / 4096,
            PQ_C2 = 2413. / 4096 * 32, PQ_C3 = 2392. / 4096 * 32;

#define MP_REF_WHITE 203.0
#define MP_REF_WHITE_HLG 3.17955

void main()
{
    vec3 yuv;
    vec3 rgb;

    yuv.x = texture(tex_y, TexCord).r;
    yuv.yz = texture(tex_uv, TexCord).rg;

    yuv += offset;
    rgb = yuv * colorConversion;

    vec4 color = vec4(rgb, 1.0);
    // ------------------
    color.rgb = clamp(color.rgb, 0.0, 1.0);
    color.rgb = pow(color.rgb, vec3(1.0 / PQ_M2));
    color.rgb = max(color.rgb - vec3(PQ_C1), vec3(0.0)) / (vec3(PQ_C2) - vec3(PQ_C3) * color.rgb);
    color.rgb = pow(color.rgb, vec3(1.0 / PQ_M1));
    // ------------------
    color.rgb = clamp(color.rgb, 0.0, 1.0);
    color.rgb = pow(color.rgb, vec3(PQ_M1));
    color.rgb = (vec3(PQ_C1) + vec3(PQ_C2) * color.rgb) / (vec3(1.0) + vec3(PQ_C3) * color.rgb);
    color.rgb = pow(color.rgb, vec3(PQ_M2));
    // ------------------
    rgb = color.rgb;

    FragColor = vec4(rgb, 1.0);
}
```

### How to achieve image quality enhancement when rendering images with OpenGL?

### Ffmpeg (5.0) decodes subtitles differently from 4.4.3

#### Decode subtitles (ffmpeg-n5.0)

```bash
0,,en,,0000,0000,0000,,Peek-a-boo!
```

you have to use`ass_process_chunk`and set pts and duration as in libavfilter/vf_subtitles.c.

#### The ASS standard format should be (ffmpeg-n4.4.3)

```bash
Dialogue: 0,0:01:06.77,0:01:08.00,en,,0000,0000,0000,,Peek-a-boo!\r\n
```

use`ass_process_data`;

### Issue with subtitle display timing when using subtitle filter

```bash
subtitles=filename='%1':original_size=%2x%3
```

## QFfmpegTranscoder

How to set encoding parameters to get smaller files and better video quality?

1.  Set a very high bitrate;

2.  Set up the encoder`global_quality`invalid. code show as below:

    ```C++
    d_ptr->codecCtx->flags |= AV_CODEC_FLAG_QSCALE;
    d_ptr->codecCtx->global_quality = FF_QP2LAMBDA * quailty;
    ```

3.  set up`crf`invalid. code show as below:

    ```C++
    av_opt_set_int(d_ptr->codecCtx, "crf", crf, AV_OPT_SEARCH_CHILDREN);
    ```

### How to calculate pts from frames obtained by AVAudioFifo?

```C++
// fix me?
frame->pts = transcodeCtx->audioPts / av_q2d(transcodeCtx->decContextInfoPtr->timebase())
                     / transcodeCtx->decContextInfoPtr->codecCtx()->sampleRate();
transcodeCtx->audioPts += frame->nb_samples;
```

### [New BING’s video transcoding recommendations](./doc/bing_transcode.md)

## SwsContext is great! Compared to QImage convert to and scale

## QT-BUG

#### Dynamically switching Video Render, switching from opengl to widget, still consumes GPU 0-3D, and the usage is twice that of opengl! ! ! QT-BUG?

### QOpenGLWidget memory leaks, moves to zoom in and out the window, the code is as follows

```C++
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setCentralWidget(new QOpenGLWidget(this));
}

```
