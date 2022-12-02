#version 330 core
in  vec2 TexCord;                // 纹理坐标
uniform int      format = -1;    // 像素格式
uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;
uniform sampler2D tex_uv;
uniform sampler2D tex_rgba;

//******************************code form SDL***********************
////#define JPEG_SHADER_CONSTANTS
//// YUV offset \n"
//const vec3 offset = vec3(0, -0.501960814, -0.501960814);

//// RGB coefficients \n"
//const vec3 Rcoeff = vec3(1,  0.000,  1.402);
//const vec3 Gcoeff = vec3(1, -0.3441, -0.7141);
//const vec3 Bcoeff = vec3(1,  1.772,  0.000);

////#define BT601_SHADER_CONSTANTS
//// YUV offset \n"
//const vec3 offset = vec3(-0.0627451017, -0.501960814, -0.501960814);

//// RGB coefficients \n"
//const vec3 Rcoeff = vec3(1.1644,  0.000,  1.596);
//const vec3 Gcoeff = vec3(1.1644, -0.3918, -0.813);
//const vec3 Bcoeff = vec3(1.1644,  2.0172,  0.000);

//#define BT709_SHADER_CONSTANTS
// YUV offset \n"
const vec3 offset = vec3(-0.0627451017, -0.501960814, -0.501960814);

// RGB coefficients \n"
const vec3 Rcoeff = vec3(1.1644,  0.000,  1.7927);
const vec3 Gcoeff = vec3(1.1644, -0.2132, -0.5329);
const vec3 Bcoeff = vec3(1.1644,  2.1124,  0.000);
//**************************************************************

void main()
{
    vec3 yuv;
    vec3 rgb;

    if(format == 0) {          // YUV420P
        yuv.x = texture2D(tex_y, TexCord).r;
        yuv.y = texture2D(tex_u, TexCord).r-0.5;
        yuv.z = texture2D(tex_v, TexCord).r-0.5;
    } else if(format == 23) {    // NV12
        yuv.x = texture2D(tex_y, TexCord.st).r - 16./256;
        yuv.y = texture2D(tex_uv, TexCord.st).r - 0.5;
        yuv.z = texture2D(tex_uv, TexCord.st).a - 0.5;
    } else if(format == 25 ){ // ARGB
        gl_FragColor = texture(tex_rgba, TexCord).gbar;
        return;
    }  else if(format == 26 ){ // RGBA
        gl_FragColor = texture(tex_rgba, TexCord);
        return;
    }  else if(format == 27){ // ABGR
        gl_FragColor = texture(tex_rgba, TexCord).abgr;
        return;
    }  else if(format == 28){ // BGRA
        gl_FragColor = texture(tex_rgba, TexCord).bgra;
        return;
    } else if(format == 120){ // 0RGB
        gl_FragColor = vec4(texture(tex_rgba, TexCord).gba, 1);
        return;
    } else if(format == 121){ // RGB0
        gl_FragColor = vec4(texture(tex_rgba, TexCord).rgb, 1);
        return;
    } else if(format == 122){ // 0BGR
        gl_FragColor = vec4(texture(tex_rgba, TexCord).abg, 1);
        return;
    } else if(format == 123){ // BGR0
        gl_FragColor = vec4(texture(tex_rgba, TexCord).bgr, 1);
        return;
    } else {}

    //    rgb = mat3(1,       1,         1,
    //               0,       -0.39465,  2.03211,
    //               1.13983, -0.58060,  0) * yuv;

    //yuv += offset;
    rgb.r = dot(yuv, Rcoeff);
    rgb.g = dot(yuv, Gcoeff);
    rgb.b = dot(yuv, Bcoeff);

    gl_FragColor = vec4(rgb, 1.0);
}
