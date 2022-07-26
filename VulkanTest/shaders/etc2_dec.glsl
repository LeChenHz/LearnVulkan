R"(
#version 430

// #define DGL_COMPRESSED_RGB8_ETC2
// #define DGL_COMPRESSED_RGBA8_ETC2_EAC
// #define DGL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
// #define DGL_COMPRESSED_SRGB8_ETC2
// #define DGL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
// #define DGL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2

%(DEFINE)

#ifdef DGL_COMPRESSED_RGB8_ETC2
#endif

#ifdef DGL_COMPRESSED_RGBA8_ETC2_EAC
#define SEPERATE_ALPHA
#endif

#ifdef DGL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
#define PUNCHTHROUGH_ALPHA
#endif

#ifdef DGL_COMPRESSED_SRGB8_ETC2
#define SRGB
#endif

#ifdef DGL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
#define SRGB
#define SEPERATE_ALPHA
#endif

#ifdef DGL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2
#define SRGB
#define PUNCHTHROUGH_ALPHA
#endif

layout(local_size_x = 4, local_size_y = 4) in;
layout(rgba8, binding = 0) uniform image2D tex;

uniform ivec2 offset;

layout(std430, binding = 0) readonly buffer InData {
    uint Raw[];
};


struct Header {
    uint mode;
#ifdef SEPERATE_ALPHA
    uint R[4];
    int alphaBase;
    int alphaMultiplier;
    int alphaModifier[8];
#else 
    uint R[2];
#endif
    
#ifdef PUNCHTHROUGH_ALPHA
    uint opacity;
#endif
    uint flip;
    uvec3 baseColors[4];
    int offsetValues[2][2];
};

shared Header H;


uint getBit(uint v, int offset, int length) {
    return (v >> offset) & ((1 << length) - 1);
}

uint testbit(uint v, int bit) {
    return v & (1 << bit);
}

void extend_4to8bits(inout uvec3 c) {
    c = c | (c << 4);
}

void extend_5to8bits(inout uvec3 c) {
    c = ((c << 3) | ((c >> 2) & 7U));
}

void extend_676to8bits(inout uvec3 c) {
    c.rb = (c.rb << 2) | ((c.rb >> 4) & 3U);
    c.g = (c.g << 1) | (c.g >> 6);
}

uint get_mode_t_h_distance(uint index) {
    switch (index) {
    case 0: return 3;
    case 1: return 6;
    case 2: return 11;
    case 3: return 16;
    case 4: return 23;
    case 5: return 32;
    case 6: return 41;
    case 7: return 64;
    }
}

void fill_offset_value(int idx, uint tb) {
    switch (tb) {
    case 0:
        H.offsetValues[idx][0] = 2;
        H.offsetValues[idx][1] = 8;
        break;
    case 1:
        H.offsetValues[idx][0] = 5;
        H.offsetValues[idx][1] = 17;
        break;
    case 2:
        H.offsetValues[idx][0] = 9;
        H.offsetValues[idx][1] = 29;
        break;
    case 3:
        H.offsetValues[idx][0] = 13;
        H.offsetValues[idx][1] = 42;
        break;
    case 4:
        H.offsetValues[idx][0] = 18;
        H.offsetValues[idx][1] = 60;
        break;
    case 5:
        H.offsetValues[idx][0] = 24;
        H.offsetValues[idx][1] = 80;
        break;
    case 6:
        H.offsetValues[idx][0] = 33;
        H.offsetValues[idx][1] = 106;
        break;
    case 7:
        H.offsetValues[idx][0] = 47;
        H.offsetValues[idx][1] = 183;
        break;
    }

#ifdef PUNCHTHROUGH_ALPHA
    if (H.opacity == 0) {
        H.offsetValues[idx][0] = 0;
    }
#endif
}


uint revert_bytes(uint r1) {
    return ((r1 & 255) << 24) | 
        (((r1 >> 8) & 255) << 16) | 
        (((r1 >> 16) & 255) << 8) | 
        (((r1 >> 24) & 255));
}

void fill_rgb() {
#ifdef PUNCHTHROUGH_ALPHA
    H.opacity = getBit(H.R[1], 1, 1);
#else
    H.mode = getBit(H.R[1], 1, 1);
    if (H.mode == 0) {
        // individial
        H.baseColors[0].r = getBit(H.R[1], 28, 4);
        H.baseColors[0].g = getBit(H.R[1], 20, 4);
        H.baseColors[0].b = getBit(H.R[1], 12, 4);
        H.baseColors[1].r = getBit(H.R[1], 24, 4);
        H.baseColors[1].g = getBit(H.R[1], 16, 4);
        H.baseColors[1].b = getBit(H.R[1], 8, 4);

        extend_4to8bits(H.baseColors[0]);
        extend_4to8bits(H.baseColors[1]);
        fill_offset_value(1, getBit(H.R[1], 2, 3));
        fill_offset_value(0, getBit(H.R[1], 5, 3));
        H.flip = getBit(H.R[1], 0, 1);
        return;
    }
#endif
    uvec3 base;
    base.r = getBit(H.R[1], 27, 5);
    base.g = getBit(H.R[1], 19, 5);
    base.b = getBit(H.R[1], 11, 5);

    uvec3 diff;
    diff.r = getBit(H.R[1], 24, 3);
    diff.g = getBit(H.R[1], 16, 3);
    diff.b = getBit(H.R[1], 8, 3);

    uvec3 bd = base + diff;
    if ((diff.r < 4 && bd.r >= 32) || 
        (diff.r >= 4 && (bd.r < 8 || bd.r >= 40))) {
        H.mode = 2; // T

        H.baseColors[0].r = getBit(H.R[1], 27, 2) << 2 | getBit(H.R[1], 24, 2);
        H.baseColors[0].g = getBit(H.R[1], 20, 4);
        H.baseColors[0].b = getBit(H.R[1], 16, 4);

        H.baseColors[2].r = getBit(H.R[1], 12, 4);
        H.baseColors[2].g = getBit(H.R[1], 8, 4);
        H.baseColors[2].b = getBit(H.R[1], 4, 4);

        extend_4to8bits(H.baseColors[0]);
        extend_4to8bits(H.baseColors[2]);

        uint distance = get_mode_t_h_distance(getBit(H.R[1], 2, 2) << 1 | getBit(H.R[1], 0, 1));
        H.baseColors[1] = H.baseColors[2] + uvec3(distance, distance, distance);
        H.baseColors[1] = clamp(H.baseColors[1], uvec3(0, 0, 0), uvec3(255, 255, 255));
        int d = int(distance);
        H.baseColors[3] = uvec3(clamp(ivec3(H.baseColors[2]) - ivec3(d, d, d), ivec3(0, 0, 0), ivec3(255, 255, 255)));
    } else if ((diff.g < 4 && bd.g >= 32) || 
        (diff.g >= 4 && (bd.g < 8 || bd.g >= 40))) {
        H.mode = 3; // H

        uvec3 c1, c2;
        c1.r = getBit(H.R[1], 27, 4);
        c1.g = (getBit(H.R[1], 24, 3) << 1) | getBit(H.R[1], 20, 1);
        c1.b = (getBit(H.R[1], 19, 1) << 3) | getBit(H.R[1], 15, 3);

        c2.r = getBit(H.R[1], 11, 4);
        c2.g = getBit(H.R[1], 7, 4);
        c2.b = getBit(H.R[1], 3, 4);

        extend_4to8bits(c1);
        extend_4to8bits(c2);

        uint dis_idx = 
            ((c1.r << 16) | (c1.r << 8) | c1.b) >=
            ((c2.r << 16) | (c2.r << 8) | c2.b) ? 1 : 0; 
        dis_idx = (getBit(H.R[1], 2, 1) << 2) | (getBit(H.R[1], 0, 1) << 1) | dis_idx;

        uint d = get_mode_t_h_distance(dis_idx);
        int id = int(d);
        H.baseColors[0] = clamp(c1 + uvec3(d, d, d), uvec3(0, 0, 0), uvec3(255, 255, 255));
        H.baseColors[1] = uvec3(clamp(ivec3(c1) - ivec3(id, id, id), ivec3(0, 0, 0), ivec3(255, 255, 255)));
        H.baseColors[2] = clamp(c2 + uvec3(d, d, d), uvec3(0, 0, 0), uvec3(255, 255, 255));
        H.baseColors[3] = uvec3(clamp(ivec3(c2) - ivec3(id, id, id), ivec3(0, 0, 0), ivec3(255, 255, 255)));

    } else if ((diff.b < 4 && bd.b >= 32) ||
        (diff.b >= 4 && (bd.b < 8 || bd.b >= 40))) {
        H.mode = 4; // Planer

        H.baseColors[0].r = getBit(H.R[1], 25, 6);
        H.baseColors[0].g = getBit(H.R[1], 24, 1) << 6 | getBit(H.R[1], 17, 6);
        H.baseColors[0].b = getBit(H.R[1], 16, 1) << 5 | getBit(H.R[1], 11, 2) << 3 | getBit(H.R[1], 7, 3);

        H.baseColors[1].r = getBit(H.R[1], 2, 5) << 1 | getBit(H.R[1], 0, 1) ;
        H.baseColors[1].g = getBit(H.R[0], 25, 7);
        H.baseColors[1].b = getBit(H.R[0], 19, 6);

        H.baseColors[2].r = getBit(H.R[0], 13, 6) ;
        H.baseColors[2].g = getBit(H.R[0], 6, 7);
        H.baseColors[2].b = getBit(H.R[0], 0, 6);

        extend_676to8bits(H.baseColors[0]);
        extend_676to8bits(H.baseColors[1]);
        extend_676to8bits(H.baseColors[2]);

    } else {
        H.mode = 1; // diff
        H.baseColors[0] = base;
        H.baseColors[1] = bd;
        if (diff.r > 3) {
            H.baseColors[1].r = H.baseColors[1].r - 8;
        }
        if (diff.g > 3) {
            H.baseColors[1].g = H.baseColors[1].g - 8;
        }
        if (diff.b > 3) {
            H.baseColors[1].b = H.baseColors[1].b - 8;
        }
        extend_5to8bits(H.baseColors[0]);
        extend_5to8bits(H.baseColors[1]);

        fill_offset_value(1, getBit(H.R[1], 2, 3));
        fill_offset_value(0, getBit(H.R[1], 5, 3));
        H.flip = getBit(H.R[1], 0, 1);
    } 
}

#ifdef SEPERATE_ALPHA
#define SET_ALPHA_MODIFIER(idx, a0, a1, a2, a3, a4, a5, a6, a7) case idx: \
    H.alphaModifier[0] = a0; H.alphaModifier[1] = a1; H.alphaModifier[2] = a2; \
    H.alphaModifier[3] = a3; H.alphaModifier[4] = a4; H.alphaModifier[5] = a5; \
    H.alphaModifier[6] = a6; H.alphaModifier[7] = a7; break;

void fill_alpha() {
    H.alphaBase = int(getBit(H.R[3], 24, 8));
    H.alphaMultiplier = int(getBit(H.R[3], 20, 4));
    uint tbidx = getBit(H.R[3], 16, 4);

    switch (tbidx) {
        SET_ALPHA_MODIFIER(0,-3,-6,-9,-15,2,5,8,14)
        SET_ALPHA_MODIFIER(1,-3,-7,-10,-13,2,6,9,12)
        SET_ALPHA_MODIFIER(2,-2,-5,-8,-13,1,4,7,12)
        SET_ALPHA_MODIFIER(3,-2,-4,-6,-13,1,3,5,12)
        SET_ALPHA_MODIFIER(4,-3,-6,-8,-12,2,5,7,11)
        SET_ALPHA_MODIFIER(5,-3,-7,-9,-11,2,6,8,10)
        SET_ALPHA_MODIFIER(6,-4,-7,-8,-11,3,6,7,10)
        SET_ALPHA_MODIFIER(7,-3,-5,-8,-11,2,4,7,10)
        SET_ALPHA_MODIFIER(8,-2,-6,-8,-10,1,5,7,9)
        SET_ALPHA_MODIFIER(9,-2,-5,-8,-10,1,4,7,9)
        SET_ALPHA_MODIFIER(10,-2,-4,-8,-10,1,3,7,9)
        SET_ALPHA_MODIFIER(11,-2,-5,-7,-10,1,4,6,9)
        SET_ALPHA_MODIFIER(12,-3,-4,-7,-10,2,3,6,9)
        SET_ALPHA_MODIFIER(13,-1,-2,-3,-10,0,1,2,9)
        SET_ALPHA_MODIFIER(14,-4,-6,-8,-9,3,5,7,8)
        SET_ALPHA_MODIFIER(15,-3,-5,-7,-9,2,4,6,8)
    }
}
#endif

void fill_header() {
    uint block_idx = gl_NumWorkGroups.x* gl_WorkGroupID.y + gl_WorkGroupID.x;

#ifdef SEPERATE_ALPHA
    H.R[3] = revert_bytes(Raw[block_idx * 4]);
    H.R[2] = revert_bytes(Raw[block_idx * 4 + 1]);
    H.R[1] = revert_bytes(Raw[block_idx * 4 + 2]);
    H.R[0] = revert_bytes(Raw[block_idx * 4 + 3]);
#else
    H.R[0] = revert_bytes(Raw[block_idx * 2 + 1]);
    H.R[1] = revert_bytes(Raw[block_idx * 2]);
#endif

    fill_rgb();

#ifdef SEPERATE_ALPHA
    fill_alpha();
#endif
   
}

void main() {
    int localidx = int(gl_LocalInvocationID.x * gl_WorkGroupSize.y + gl_LocalInvocationID.y);
    if (localidx == 0) {
        fill_header();
        memoryBarrierShared();
    }
    barrier();

     

    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy) + offset;

    uint lsb = getBit(H.R[0], localidx, 1);
    uint msb = getBit(H.R[0], localidx + 16, 1); 

#ifdef PUNCHTHROUGH_ALPHA
    if (H.mode != 4 && H.opacity == 0 && msb == 1 && lsb == 0) {
        imageStore(tex, pixel_coords, vec4(0.0, 0.0, 0.0, 0.0));
        return;
    } 
#endif
    ivec3 color;
    if (H.mode < 2) {
        // diff/individial
        uint blockidx;
        if (H.flip == 0) {
            blockidx = gl_LocalInvocationID.x < 2 ? 0 : 1;
        } else {
            blockidx = gl_LocalInvocationID.y < 2 ? 0 : 1;
        }
        int off = H.offsetValues[blockidx][lsb];
        if (msb != 0) {
            off = off * -1;
        }
        color = ivec3(H.baseColors[blockidx]) + ivec3(off, off, off);
    } else if (H.mode < 4) {
        // T/H
        uint pidx = lsb | (msb << 1);
        color = ivec3(H.baseColors[pidx]);
    } else {
        // planer
        color = (int(gl_LocalInvocationID.x) * (ivec3(H.baseColors[1]) - ivec3(H.baseColors[0])) + 
            int(gl_LocalInvocationID.y) * (ivec3(H.baseColors[2]) - ivec3(H.baseColors[0])) + 4 * ivec3(H.baseColors[0]) + 2) >> 2;
    }

    color = clamp(color, ivec3(0, 0, 0), ivec3(255, 255, 255));

    float alpha = 1.0;
#ifdef SEPERATE_ALPHA
    uint alphaidx;
    if (localidx < 5) {
        alphaidx = getBit(H.R[3], 13 - 3 * localidx, 3);
    } else if (localidx == 5) {
        alphaidx = (getBit(H.R[3], 0, 1) << 2) | getBit(H.R[2], 30, 2);
    } else {
        alphaidx = getBit(H.R[2], 45 - 3 * localidx, 3);
    }
    alpha = float(clamp(H.alphaBase + H.alphaModifier[alphaidx] * H.alphaMultiplier, 0, 255)) / 255.0;
#endif
    vec4 retval = vec4(float(color.r) / 255.0, float(color.g) / 255.0, float(color.b) / 255.0, alpha);
#ifdef SRGB
    retval.rgb = pow(retval.rgb, vec3(2.2, 2.2, 2.2));
#endif
    imageStore(tex, pixel_coords, retval);
}

)"
