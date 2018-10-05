
///////////////////////////
//   generic helpers

inline void swap_ints(int u[], int v[], int n)
{
    for (int i=0; i<n; i++)
    {
        int t = u[i];
        u[i] = v[i];
        v[i] = t;
    }
}

inline void swap_uints(uint u[], uint v[], int n)
{
    for (int i=0; i<n; i++)
    {
        uint t = u[i];
        u[i] = v[i];
        v[i] = t;
    }
}

inline float sq(const float sq)
{
    return sq * sq;
}

inline int pow2(int x) 
{
    return 1<<x; 
}



inline void ssymv(float a[3], float covar[6], float b[3])
{
    a[0] = covar[0]*b[0]+covar[1]*b[1]+covar[2]*b[2];
    a[1] = covar[1]*b[0]+covar[3]*b[1]+covar[4]*b[2];
    a[2] = covar[2]*b[0]+covar[4]*b[1]+covar[5]*b[2];
}

inline void ssymv3(float a[4], float covar[10], float b[4])
{
    a[0] = covar[0]*b[0]+covar[1]*b[1]+covar[2]*b[2];
    a[1] = covar[1]*b[0]+covar[4]*b[1]+covar[5]*b[2];
    a[2] = covar[2]*b[0]+covar[5]*b[1]+covar[7]*b[2];
}

inline void ssymv4(float a[4], float covar[10], float b[4])
{
    a[0] = covar[0]*b[0]+covar[1]*b[1]+covar[2]*b[2]+covar[3]*b[3];
    a[1] = covar[1]*b[0]+covar[4]*b[1]+covar[5]*b[2]+covar[6]*b[3];
    a[2] = covar[2]*b[0]+covar[5]*b[1]+covar[7]*b[2]+covar[8]*b[3];
    a[3] = covar[3]*b[0]+covar[6]*b[1]+covar[8]*b[2]+covar[9]*b[3];
}

inline void compute_axis3(float axis[3], float covar[6], const int powerIterations)
{
    float vec[3] = {1,1,1};

    for (int i=0; i<powerIterations; i++)
    {
        ssymv(axis, covar, vec);
        for (int p=0; p<3; p++) vec[p] = axis[p];

        if (i%2==1) // renormalize every other iteration
        {
            float norm_sq = 0;
            for (int p=0; p<3; p++)
                norm_sq += axis[p]*axis[p];

            float rnorm = rsqrt(norm_sq);
            for (int p=0; p<3; p++) vec[p] *= rnorm;
        }		
    }

    for (int p=0; p<3; p++) axis[p] = vec[p];
}

inline void compute_axis(float axis[4], float covar[10], const int powerIterations, int channels)
{
    float vec[4] = {1,1,1,1};

    for (int i=0; i<powerIterations; i++)
    {
        if (channels == 3) ssymv3(axis, covar, vec);
        if (channels == 4) ssymv4(axis, covar, vec);
        for (int p=0; p<channels; p++) vec[p] = axis[p];

        if (i%2==1) // renormalize every other iteration
        {
            float norm_sq = 0;
            for (int p=0; p<channels; p++)
                norm_sq += axis[p]*axis[p];

            float rnorm = rsqrt(norm_sq);
            for (int p=0; p<channels; p++) vec[p] *= rnorm;
        }		
    }

    for (int p=0; p<channels; p++) axis[p] = vec[p];
}

typedef struct bc7_enc_settings
{
    bool mode_selection[4];
    int refineIterations[8];

    bool skip_mode2;
    int fastSkipTreshold_mode1;
    int fastSkipTreshold_mode3;
    int fastSkipTreshold_mode7;

    int mode45_channel0;
    int refineIterations_channel;

    int channels;
    uint width, height, stride;
}bc7_enc_settings;

typedef struct bc7_enc_state
{
    float opaque_err;       // error for coding alpha=255
    float best_err;
    uint best_data[5];	// 4, +1 margin for skips
}bc7_enc_state;

typedef struct mode45_parameters
{
    int qep[8];
    uint qblock[2];
    int aqep[2];
    uint aqblock[2];
    int rotation;
    int swap;
}mode45_parameters;

void bc7_code_mode01237(local uint* restrict data, int* restrict qep, uint qblock[2], int part_id, int mode);
void bc7_code_mode45(local uint* restrict data, mode45_parameters* restrict params, int mode);
void bc7_code_mode6(local uint* restrict data, int qep[8], uint qblock[2]);

///////////////////////////
//   BC7 format data

constant int unquant_tables[] = 
{
    0, 21, 43, 64,
    0, 9, 18, 27, 37, 46, 55, 64,
    0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64
};

inline constant int* get_unquant_table(int bits)
{
    switch (bits)
    {
    case 2:  return &unquant_tables[0];
    case 3:  return &unquant_tables[4];
    case 4:  return &unquant_tables[12];

    default: return &unquant_tables[0];
    }
}

constant uint pattern_table[] = 
{
    0x50505050u, 0x40404040u, 0x54545454u, 0x54505040u, 0x50404000u, 0x55545450u, 0x55545040u, 0x54504000u,
    0x50400000u, 0x55555450u, 0x55544000u, 0x54400000u, 0x55555440u, 0x55550000u, 0x55555500u, 0x55000000u,
    0x55150100u, 0x00004054u, 0x15010000u, 0x00405054u, 0x00004050u, 0x15050100u, 0x05010000u, 0x40505054u,
    0x00404050u, 0x05010100u, 0x14141414u, 0x05141450u, 0x01155440u, 0x00555500u, 0x15014054u, 0x05414150u,
    0x44444444u, 0x55005500u, 0x11441144u, 0x05055050u, 0x05500550u, 0x11114444u, 0x41144114u, 0x44111144u,
    0x15055054u, 0x01055040u, 0x05041050u, 0x05455150u, 0x14414114u, 0x50050550u, 0x41411414u, 0x00141400u,
    0x00041504u, 0x00105410u, 0x10541000u, 0x04150400u, 0x50410514u, 0x41051450u, 0x05415014u, 0x14054150u,
    0x41050514u, 0x41505014u, 0x40011554u, 0x54150140u, 0x50505500u, 0x00555050u, 0x15151010u, 0x54540404u,
    0xAA685050u, 0x6A5A5040u, 0x5A5A4200u, 0x5450A0A8u, 0xA5A50000u, 0xA0A05050u, 0x5555A0A0u, 0x5A5A5050u,
    0xAA550000u, 0xAA555500u, 0xAAAA5500u, 0x90909090u, 0x94949494u, 0xA4A4A4A4u, 0xA9A59450u, 0x2A0A4250u,
    0xA5945040u, 0x0A425054u, 0xA5A5A500u, 0x55A0A0A0u, 0xA8A85454u, 0x6A6A4040u, 0xA4A45000u, 0x1A1A0500u,
    0x0050A4A4u, 0xAAA59090u, 0x14696914u, 0x69691400u, 0xA08585A0u, 0xAA821414u, 0x50A4A450u, 0x6A5A0200u,
    0xA9A58000u, 0x5090A0A8u, 0xA8A09050u, 0x24242424u, 0x00AA5500u, 0x24924924u, 0x24499224u, 0x50A50A50u,
    0x500AA550u, 0xAAAA4444u, 0x66660000u, 0xA5A0A5A0u, 0x50A050A0u, 0x69286928u, 0x44AAAA44u, 0x66666600u,
    0xAA444444u, 0x54A854A8u, 0x95809580u, 0x96969600u, 0xA85454A8u, 0x80959580u, 0xAA141414u, 0x96960000u,
    0xAAAA1414u, 0xA05050A0u, 0xA0A5A5A0u, 0x96000000u, 0x40804080u, 0xA9A8A9A8u, 0xAAAAAA44u, 0x2A4A5254u
};
inline uint get_pattern(const int part_id)
{
    return pattern_table[part_id];
}

constant uint pattern_mask_table[] = 
{
    0xCCCC3333u, 0x88887777u, 0xEEEE1111u, 0xECC81337u, 0xC880377Fu, 0xFEEC0113u, 0xFEC80137u, 0xEC80137Fu,
    0xC80037FFu, 0xFFEC0013u, 0xFE80017Fu, 0xE80017FFu, 0xFFE80017u, 0xFF0000FFu, 0xFFF0000Fu, 0xF0000FFFu,
    0xF71008EFu, 0x008EFF71u, 0x71008EFFu, 0x08CEF731u, 0x008CFF73u, 0x73108CEFu, 0x3100CEFFu, 0x8CCE7331u,
    0x088CF773u, 0x3110CEEFu, 0x66669999u, 0x366CC993u, 0x17E8E817u, 0x0FF0F00Fu, 0x718E8E71u, 0x399CC663u,
    0xAAAA5555u, 0xF0F00F0Fu, 0x5A5AA5A5u, 0x33CCCC33u, 0x3C3CC3C3u, 0x55AAAA55u, 0x96966969u, 0xA55A5AA5u,
    0x73CE8C31u, 0x13C8EC37u, 0x324CCDB3u, 0x3BDCC423u, 0x69969669u, 0xC33C3CC3u, 0x99666699u, 0x0660F99Fu,
    0x0272FD8Du, 0x04E4FB1Bu, 0x4E40B1BFu, 0x2720D8DFu, 0xC93636C9u, 0x936C6C93u, 0x39C6C639u, 0x639C9C63u,
    0x93366CC9u, 0x9CC66339u, 0x817E7E81u, 0xE71818E7u, 0xCCF0330Fu, 0x0FCCF033u, 0x774488BBu, 0xEE2211DDu,
    0x08CC0133u, 0x8CC80037u, 0xCC80006Fu, 0xEC001331u, 0x330000FFu, 0x00CC3333u, 0xFF000033u, 0xCCCC0033u,
    0x0F0000FFu, 0x0FF0000Fu, 0x00F0000Fu, 0x44443333u, 0x66661111u, 0x22221111u, 0x136C0013u, 0x008C8C63u,
    0x36C80137u, 0x08CEC631u, 0x3330000Fu, 0xF0000333u, 0x00EE1111u, 0x88880077u, 0x22C0113Fu, 0x443088CFu,
    0x0C22F311u, 0x03440033u, 0x69969009u, 0x9960009Fu, 0x03303443u, 0x00660699u, 0xC22C3113u, 0x8C0000EFu,
    0x1300007Fu, 0xC4003331u, 0x004C1333u, 0x22229999u, 0x00F0F00Fu, 0x24929249u, 0x29429429u, 0xC30C30C3u,
    0xC03C3C03u, 0x00AA0055u, 0xAA0000FFu, 0x30300303u, 0xC0C03333u, 0x90900909u, 0xA00A5005u, 0xAAA0000Fu,
    0x0AAA0555u, 0xE0E01111u, 0x70700707u, 0x6660000Fu, 0x0EE01111u, 0x07707007u, 0x06660999u, 0x660000FFu,
    0x00660099u, 0x0CC03333u, 0x03303003u, 0x60000FFFu, 0x80807777u, 0x10100101u, 0x000A0005u, 0x08CE8421u
};
inline int get_pattern_mask(const int part_id, const int j)
{
    uint mask_packed = pattern_mask_table[part_id];
    int mask0 = mask_packed&0xFFFF;
    int mask1 = mask_packed>>16;

    int mask = (j==2) ? (~mask0)&(~mask1) : ( (j==0) ? mask0 : mask1 );
    return mask;
}

constant uchar skip_table[] = 
{
    0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 
    0xf0u, 0x20u, 0x80u, 0x20u, 0x20u, 0x80u, 0x80u, 0xf0u, 0x20u, 0x80u, 0x20u, 0x20u, 0x80u, 0x80u, 0x20u, 0x20u,
    0xf0u, 0xf0u, 0x60u, 0x80u, 0x20u, 0x80u, 0xf0u, 0xf0u, 0x20u, 0x80u, 0x20u, 0x20u, 0x20u, 0xf0u, 0xf0u, 0x60u, 
    0x60u, 0x20u, 0x60u, 0x80u, 0xf0u, 0xf0u, 0x20u, 0x20u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0xf0u, 0x20u, 0x20u, 0xf0u,
    0x3fu, 0x38u, 0xf8u, 0xf3u, 0x8fu, 0x3fu, 0xf3u, 0xf8u, 0x8fu, 0x8fu, 0x6fu, 0x6fu, 0x6fu, 0x5fu, 0x3fu, 0x38u, 
    0x3fu, 0x38u, 0x8fu, 0xf3u, 0x3fu, 0x38u, 0x6fu, 0xa8u, 0x53u, 0x8fu, 0x86u, 0x6au, 0x8fu, 0x5fu, 0xfau, 0xf8u,
    0x8fu, 0xf3u, 0x3fu, 0x5au, 0x6au, 0xa8u, 0x89u, 0xfau, 0xf6u, 0x3fu, 0xf8u, 0x5fu, 0xf3u, 0xf6u, 0xf6u, 0xf8u, 
    0x3fu, 0xf3u, 0x5fu, 0x5fu, 0x5fu, 0x8fu, 0x5fu, 0xafu, 0x5fu, 0xafu, 0x8fu, 0xdfu, 0xf3u, 0xcfu, 0x3fu, 0x38u
};
inline uchar3 get_skips(int part_id)
{
    int skip_packed = skip_table[part_id];
    return (uchar3)(0, skip_packed>>4, skip_packed&15);
}


///////////////////////////
//      PCA helpers

inline void compute_stats_masked(float stats[15], local float* restrict block, int mask, int channels)
{
    for (int i=0; i<15; i++) stats[i] = 0;

    int mask_shifted = mask<<1;
    for (int k=0; k<16; k++)
    {
        mask_shifted >>= 1;
        //if ((mask_shifted&1) == 0) continue;
        int flag = (mask_shifted&1);

        float rgba[4];
        for (int p=0; p<channels; p++) rgba[p] = block[k+p*16];
        
        for (int p=0; p<channels; p++) rgba[p] *= flag;
        stats[14] += flag;

        stats[10] += rgba[0];
        stats[11] += rgba[1];
        stats[12] += rgba[2];

        stats[0] += rgba[0]*rgba[0];
        stats[1] += rgba[0]*rgba[1];
        stats[2] += rgba[0]*rgba[2];

        stats[4] += rgba[1]*rgba[1];
        stats[5] += rgba[1]*rgba[2];

        stats[7] += rgba[2]*rgba[2];

        if (channels==4)
        {
            stats[13] += rgba[3];

            stats[3] += rgba[0]*rgba[3];
            stats[6] += rgba[1]*rgba[3];
            stats[8] += rgba[2]*rgba[3];
            stats[9] += rgba[3]*rgba[3];
        }
    }
}

inline void covar_from_stats(float covar[10], float stats[15], int channels)
{
    covar[0] = stats[0] - stats[10+0]*stats[10+0]/stats[14];
    covar[1] = stats[1] - stats[10+0]*stats[10+1]/stats[14];
    covar[2] = stats[2] - stats[10+0]*stats[10+2]/stats[14];

    covar[4] = stats[4] - stats[10+1]*stats[10+1]/stats[14];
    covar[5] = stats[5] - stats[10+1]*stats[10+2]/stats[14];

    covar[7] = stats[7] - stats[10+2]*stats[10+2]/stats[14];

    if (channels == 4)
    {
        covar[3] = stats[3] - stats[10+0]*stats[10+3]/stats[14];
        covar[6] = stats[6] - stats[10+1]*stats[10+3]/stats[14];
        covar[8] = stats[8] - stats[10+2]*stats[10+3]/stats[14];
        covar[9] = stats[9] - stats[10+3]*stats[10+3]/stats[14];
    }
}

inline void compute_covar_dc_masked(float covar[6], float dc[3], local float* restrict block, int mask, int channels)
{
    float stats[15];
    compute_stats_masked(stats, block, mask, channels);

    covar_from_stats(covar, stats, channels);
    for (int p=0; p<channels; p++) dc[p] = stats[10+p]/stats[14];
}

void block_pca_axis(float axis[4], float dc[4], local float* restrict block, int mask, int channels)
{
    const int powerIterations = 8; // 4 not enough for HQ

    float covar[10];
    compute_covar_dc_masked(covar, dc, block, mask, channels);

    //float var = covar[0] + covar[4] + covar[7] + covar[9] + 256;
    float inv_var = 1.0f / (256 * 256);
    for (int k = 0; k < 10; k++)
    {
        covar[k] *= inv_var;
    }

    float eps = 1e-6f;
    covar[0] += eps;
    covar[4] += eps;
    covar[7] += eps;
    covar[9] += eps;

    compute_axis(axis, covar, powerIterations, channels);
}

void block_segment_core(private float* restrict ep, local float* restrict block, int mask, uchar channels)
{
    float axis[4];
    float dc[4];
    block_pca_axis(axis, dc, block, mask, channels);
    
    float ext[2];
    ext[0] = +1e30f;
    ext[1] = -1e30f;

    // find min/max
    int mask_shifted = mask<<1;
    for (int k=0; k<16; k++)
    {
        mask_shifted >>= 1;
        if ((mask_shifted&1) == 0) continue;

        float dot = 0;
        for (int p=0; p<channels; p++)
            dot += axis[p]*(block[16*p+k]-dc[p]);

        ext[0] = min(ext[0], dot);
        ext[1] = max(ext[1], dot);
    }

    // create some distance if the endpoints collapse
    if (ext[1]-ext[0] < 1.f)
    {
        ext[0] -= 0.5f;
        ext[1] += 0.5f;
    }

    for (int i=0; i<2; i++)
    for (int p=0; p<channels; p++)
    {
        ep[4*i+p] = ext[i]*axis[p]+dc[p];
    }
}

void block_segment(private float* restrict ep, local float* restrict block, int mask, uchar channels)
{
    block_segment_core(ep, block, mask, channels);

    for (uchar p=0; p<channels; p++)
    {
        ep[4+p] = clamp(ep[4+p], 0.f, 255.f);
        ep[p] = clamp(ep[p], 0.f, 255.f);
    }
}

float get_pca_bound(float covar[10], int channels)
{
    const int powerIterations = 4; // quite approximative, but enough for bounding

    float inv_var = 1.0f / (256 * 256);
    for (int k = 0; k < 10; k++)
    {
        covar[k] *= inv_var;
    }

    float eps = 1e-6f;
    covar[0] += eps;
    covar[4] += eps;
    covar[7] += eps;

    float axis[4];
    compute_axis(axis, covar, powerIterations, channels);

    float vec[4];
    if (channels == 3) ssymv3(vec, covar, axis);
    if (channels == 4) ssymv4(vec, covar, axis);

    float sq_sum = 0.f;
    for (int p=0; p<channels; p++) sq_sum += sq(vec[p]);
    float lambda = sqrt(sq_sum);

    float bound = covar[0]+covar[4]+covar[7];
    if (channels == 4) bound += covar[9];
    bound -= lambda;
    bound = max(bound, 0.0f);

    return bound;
}

float block_pca_bound(local float* restrict block, int mask, int channels)
{
    float stats[15];
    compute_stats_masked(stats, block, mask, channels);

    float covar[10];
    covar_from_stats(covar, stats, channels);

    return get_pca_bound(covar, channels);
}

float block_pca_bound_split(local float* restrict block, int mask, float full_stats[15], int channels)
{
    float stats[15];
    compute_stats_masked(stats, block, mask, channels);
    
    float covar1[10];
    covar_from_stats(covar1, stats, channels);
    
    for (int i=0; i<15; i++)
        stats[i] = full_stats[i] - stats[i];

    float covar2[10];
    covar_from_stats(covar2, stats, channels);

    float bound = 0.f;
    bound += get_pca_bound(covar1, channels);
    bound += get_pca_bound(covar2, channels);

    return sqrt(bound)*256;
}

///////////////////////////
// endpoint quantization

inline int unpack_to_byte(int v, const uchar bits)
{
    int vv = v << (8-bits);
    return vv + (vv >> bits);
}

void ep_quant0367(int qep[], float ep[], int mode, int channels)
{
    int bits = 7;
    if (mode == 0) bits = 4;
    if (mode == 7) bits = 5;

    int levels = 1 << bits;
    int levels2 = levels*2-1;
        
    for (int i=0; i<2; i++)
    {
        int qep_b[8];
    
        for (int b=0; b<2; b++)
        for (int p=0; p<4; p++)
        {
            int v = (int)((ep[i*4+p]/255.f*levels2-b)/2+0.5f)*2+b;
            qep_b[b*4+p] = clamp(v, b, levels2-1+b);
        }

        float ep_b[8];
        for (int j=0; j<8; j++)
            ep_b[j] = qep_b[j];

        if (mode==0)
        for (int j=0; j<8; j++)
            ep_b[j] = unpack_to_byte(qep_b[j], 5);
    
        float err0 = 0.f;
        float err1 = 0.f;
        for (int p=0; p<channels; p++)
        {
            err0 += sq(ep[i*4+p]-ep_b[0+p]);
            err1 += sq(ep[i*4+p]-ep_b[4+p]);
        }

        for (int p=0; p<4; p++)
            qep[i*4+p] = (err0<err1) ? qep_b[0+p] : qep_b[4+p];
    }
}

void ep_quant1(int qep[], float ep[], int mode)
{
    int qep_b[16];
        
    for (int b=0; b<2; b++)
    for (int i=0; i<8; i++)
    {
        int v = ((int)((ep[i]/255.f*127.f-b)/2+0.5f))*2+b;
        qep_b[b*8+i] = clamp(v, b, 126+b);
    }
    
    // dequant
    float ep_b[16];
    for (int k=0; k<16; k++)
        ep_b[k] = unpack_to_byte(qep_b[k], 7);

    float err0 = 0.f;
    float err1 = 0.f;
    for (int j = 0; j < 2; j++)
    for (int p = 0; p < 3; p++)
    {
        err0 += sq(ep[j * 4 + p] - ep_b[0 + j * 4 + p]);
        err1 += sq(ep[j * 4 + p] - ep_b[8 + j * 4 + p]);
    }

    for (int i=0; i<8; i++)
        qep[i] = (err0<err1) ? qep_b[0+i] : qep_b[8+i];

}

void ep_quant245(int qep[], float ep[], int mode)
{
    int bits = 5;
    if (mode == 5) bits = 7;
    int levels = 1 << bits;
        
    for (int i=0; i<8; i++)
    {
        int v = ((int)(ep[i]/255.f*(levels-1)+0.5f));
        qep[i] = clamp(v, 0, levels-1);
    }
}

void ep_quant(int qep[], float ep[], int mode, int channels)
{
    const int pairs_table[] = {3,2,3,2,1,1,1,2};
    const int pairs = pairs_table[mode];

    if (mode == 0 || mode == 3 || mode == 6 || mode == 7)
    {
        for (int i=0; i<pairs; i++)
            ep_quant0367(&qep[i*8], &ep[i*8], mode, channels);
    }
    else if (mode == 1)
    {
        for (int i=0; i<pairs; i++)
            ep_quant1(&qep[i*8], &ep[i*8], mode);
    }
    else if (mode == 2 || mode == 4 || mode == 5)
    {
        for (int i=0; i<pairs; i++)
            ep_quant245(&qep[i*8], &ep[i*8], mode);
    }
}

void ep_dequant(float ep[], int qep[], int mode)
{
    const int pairs_table[] = {3,2,3,2,1,1,1,2};
    const int pairs = pairs_table[mode];
    
    // mode 3, 6 are 8-bit
    if (mode == 3 || mode == 6)
    {
        for (int i=0; i<8*pairs; i++)
            ep[i] = qep[i];
    }
    else if (mode == 1 || mode == 5)
    {
        for (int i=0; i<8*pairs; i++)
            ep[i] = unpack_to_byte(qep[i], 7);
    }
    else if (mode == 0 || mode == 2 || mode == 4)
    {
        for (int i=0; i<8*pairs; i++)
            ep[i] = unpack_to_byte(qep[i], 5);
    }
    else if (mode == 7)
    {
        for (int i=0; i<8*pairs; i++)
            ep[i] = unpack_to_byte(qep[i], 6);
    }
}

void ep_quant_dequant(int qep[], float ep[], int mode, int channels)
{
    ep_quant(qep, ep, mode, channels);
    ep_dequant(ep, qep, mode);
}


///////////////////////////
//   pixel quantization

float block_quant(uint qblock[2], local float* restrict block, int bits, float ep[], uint pattern, int channels)
{
    float total_err = 0;
    constant int* restrict unquant_table = get_unquant_table(bits);
    int levels = 1 << bits;

    // 64-bit qblock: 5% overhead in this function
    qblock[0] = 0;
    qblock[1] = 0;

    int pattern_shifted = pattern;
    for (int k=0; k<16; k++)
    {
        int j = pattern_shifted&3;
        pattern_shifted >>= 2;

        float proj = 0;
        float div = 0;
        for (int p=0; p<channels; p++)
        {
            float ep_a = ep[8*j+0+p];
            float ep_b = ep[8*j+4+p];
            proj += (block[k+p*16]-ep_a)*(ep_b-ep_a);
            div += sq(ep_b-ep_a);
        }
        
        proj /= div;
                
        int q1 = (int)(proj*levels+0.5f);
        q1 = clamp(q1, 1, levels-1);
        
        float err0 = 0;
        float err1 = 0;
        int w0 = unquant_table[q1-1];
        int w1 = unquant_table[q1];

        for (int p=0; p<channels; p++)
        {
            float ep_a = ep[8*j+0+p];
            float ep_b = ep[8*j+4+p];
            float dec_v0 = (int)(((64-w0)*ep_a + w0*ep_b + 32)/64);
            float dec_v1 = (int)(((64-w1)*ep_a + w1*ep_b + 32)/64);
            err0 += sq(dec_v0 - block[k+p*16]);
            err1 += sq(dec_v1 - block[k+p*16]);
        }
        
        int best_err = err1;
        int best_q = q1;
        if (err0<err1)
        {
            best_err = err0;
            best_q = q1-1;
        }

        qblock[k/8] += ((uint)best_q) << 4*(k%8);
        total_err += best_err;
    }

    return total_err;
}

///////////////////////////
// LS endpoint refinement

void opt_endpoints(float* ep, local float* restrict block, uchar bits, uint* restrict qblock, int mask, uchar channels)
{
    uchar levels = 1 << bits;
    uchar levels_1 = levels-1;
    
    float Atb1[4] = {0,0,0,0};
    float sum_q = 0;
    float sum_qq = 0;
    float sum[5] = {0,0,0,0,0};
                
    int mask_shifted = mask<<1;

    for (uchar k1=0; k1<2; k1++)
    {
        uint qbits_shifted = qblock[k1];
        for (uchar k2=0; k2<8; k2++)
        {
            int k = k1*8+k2;
            float q = (int)(qbits_shifted&15);
            qbits_shifted >>= 4;

            mask_shifted >>= 1;
            if ((mask_shifted&1) == 0) continue;
        
            int x = levels_1-q;
            int y = q;
            
            sum_q += q;
            sum_qq += q*q;

            sum[4] += 1;
            for (int p=0; p<channels; p++) sum[p] += block[k+p*16];
            for (int p=0; p<channels; p++) Atb1[p] += x*block[k+p*16];
        }
    }
        
    float Atb2[4];
    for (int p=0; p<channels; p++) 
    {
        //sum[p] = dc[p]*16;
        Atb2[p] = levels_1*sum[p]-Atb1[p];
    }
        
    float Cxx = sum[4]*levels_1*levels_1-2*levels_1*sum_q+sum_qq;
    float Cyy = sum_qq;
    float Cxy = levels_1*sum_q-sum_qq;
    float scale = levels_1 / (Cxx*Cyy - Cxy*Cxy);

    for (int p=0; p<channels; p++)
    {
        ep[0+p] = (Atb1[p]*Cyy - Atb2[p]*Cxy)*scale;
        ep[4+p] = (Atb2[p]*Cxx - Atb1[p]*Cxy)*scale;
            
        //ep[0+p] = clamp(ep[0+p], 0, 255);
        //ep[4+p] = clamp(ep[4+p], 0, 255);
    }

    if (fabs(Cxx*Cyy - Cxy*Cxy) < 0.001f)
    {
        // flatten
        for (int p=0; p<channels; p++)
        {
            ep[0+p] = sum[p]/sum[4];
            ep[4+p] = ep[0+p];
        }
    }
}


inline float bc7_enc_mode01237_part_fast(private int* restrict qep, private uint* restrict qblock, 
    local float* restrict block, int part_id, const int mode)
{
    uint pattern = get_pattern(part_id);
    uchar bits  = (mode == 0 || mode == 1) ? 3 : 2;
    uchar pairs = (mode == 0 || mode == 2) ? 3 : 2;
    uchar channels = (mode == 7) ? 4 : 3;

    float ep[24];
    for (uchar j=0; j<pairs; j++)
    {
        int mask = get_pattern_mask(part_id, j);
        block_segment(&ep[j*8], block, mask, channels);
    }

    ep_quant_dequant(qep, ep, mode, channels);

    float total_err = block_quant(qblock, block, bits, ep, pattern, channels);
    return total_err;
}


inline void bc7_enc_mode01237(local float* restrict block, local bc7_enc_state* restrict state,
    constant bc7_enc_settings* restrict settings,
    const int mode, private uchar* restrict part_list, const uchar part_count)
{
    if (part_count == 0) return;
    uchar bits  = (mode == 0 || mode == 1) ? 3 : 2;
    uchar pairs = (mode == 0 || mode == 2) ? 3 : 2;
    uchar channels = (mode == 7) ? 4 : 3;

    int best_qep[24];
    uint best_qblock[2];
    int best_part_id = -1;
    float best_err = 1e30f;

    for (uchar part=0; part<part_count; part++)
    {
        uchar part_id = part_list[part] & (uchar)63;
        if (pairs == 3) part_id += (uchar)64;

        int qep[24];
        uint qblock[2];
        float err = bc7_enc_mode01237_part_fast(qep, qblock, block, part_id, mode);
        
        if (err<best_err)
        {
            for (int i=0; i<8*pairs; i++) best_qep[i] = qep[i];
            for (int k=0; k<2; k++) best_qblock[k] = qblock[k];
            best_part_id = part_id;
            best_err = err;
        }
    }
    
    // refine
    int refineIterations = settings->refineIterations[mode];
    for (int _=0; _<refineIterations; _++)
    {
        float ep[24];
        for (int j=0; j<pairs; j++)
        {
            int mask = get_pattern_mask(best_part_id, j);
            opt_endpoints(&ep[j*8], block, bits, best_qblock, mask, channels);
        }

        int qep[24];
        uint qblock[2];

        ep_quant_dequant(qep, ep, mode, channels);
        
        uint pattern = get_pattern(best_part_id);
        float err = block_quant(qblock, block, bits, ep, pattern, channels);

        if (err<best_err)
        {
            for (int i=0; i<8*pairs; i++) best_qep[i] = qep[i];
            for (int k=0; k<2; k++) best_qblock[k] = qblock[k];
            best_err = err;
        }
    }
    
    if (mode != 7) best_err += state->opaque_err; // take into account alpha channel

    if (best_err<state->best_err)
    {
        state->best_err = best_err;
        bc7_code_mode01237(state->best_data, best_qep, best_qblock, best_part_id, mode);
    }
}

void partial_sort_list(uchar list[], int length, int partial_count)
{
    for (int k=0; k<partial_count; k++)
    {
        int best_idx = k;
        uchar best_value = list[k];
        for (int i=k+1; i<length; i++)
        {
            if (best_value > list[i])
            {
                best_value = list[i];
                best_idx = i;
            }
        }

        // swap
        list[best_idx] = list[k];
        list[k] = best_value;
    }
}

void bc7_enc_mode02(local float* restrict block, local bc7_enc_state* restrict state, constant bc7_enc_settings* restrict settings)
{
    uchar part_list[64];
    for (uchar part=0; part<64; part++)
        part_list[part] = part;

    bc7_enc_mode01237(block, state, settings, 0, part_list, 16); 
    if (!settings->skip_mode2) bc7_enc_mode01237(block, state, settings, 2, part_list, 64); // usually not worth the time
}

void bc7_enc_mode13(local float* restrict block, local bc7_enc_state* restrict state, constant bc7_enc_settings* restrict settings)
{
    if (settings->fastSkipTreshold_mode1 == 0 && settings->fastSkipTreshold_mode3 == 0) return;

    float full_stats[15];
    compute_stats_masked(full_stats, block, -1, 3);

    uchar part_list[64];
    for (int part=0; part<64; part++)
    {
        int mask = get_pattern_mask(part+0, 0);
        float bound12 = block_pca_bound_split(block, mask, full_stats, 3);
        int bound = (int)(bound12);
        part_list[part] = part+bound*64;
    }

    partial_sort_list(part_list, 64, max(settings->fastSkipTreshold_mode1, settings->fastSkipTreshold_mode3));
    bc7_enc_mode01237(block, state, settings, 1, part_list, settings->fastSkipTreshold_mode1);
    bc7_enc_mode01237(block, state, settings, 3, part_list, settings->fastSkipTreshold_mode3);
}

void bc7_enc_mode7(local float* restrict block, local bc7_enc_state* restrict state, constant bc7_enc_settings* restrict settings)
{
    if (settings->fastSkipTreshold_mode7 == 0) return;

    float full_stats[15];
    compute_stats_masked(full_stats, block, -1, settings->channels);

    uchar part_list[64];
    for (int part=0; part<64; part++)
    {
        int mask = get_pattern_mask(part+0, 0);
        float bound12 = block_pca_bound_split(block, mask, full_stats, settings->channels);
        int bound = (int)(bound12);
        part_list[part] = part+bound*64;
    }

    partial_sort_list(part_list, 64, settings->fastSkipTreshold_mode7);
    bc7_enc_mode01237(block, state, settings, 7, part_list, settings->fastSkipTreshold_mode7);
}

void channel_quant_dequant(int qep[2], float ep[2], int epbits)
{
    int elevels = (1<<epbits);

    for (int i=0; i<2; i++)
    {
        int v = ((int)(ep[i]/255.f*(elevels-1)+0.5f));
        qep[i] = clamp(v, 0, elevels-1);
        ep[i] = unpack_to_byte(qep[i], epbits);
    }
}

void channel_opt_endpoints(float ep[2], local float* restrict block, int bits, uint qblock[2])
{
    int levels = 1 << bits;

    float Atb1 = 0;
    float sum_q = 0;
    float sum_qq = 0;
    float sum = 0;
                
    for (int k1=0; k1<2; k1++)
    {
        uint qbits_shifted = qblock[k1];
        for (int k2=0; k2<8; k2++)
        {
            int k = k1*8+k2;
            float q = (int)(qbits_shifted&15);
            qbits_shifted >>= 4;

            int x = (levels-1)-q;
            int y = q;
            
            sum_q += q;
            sum_qq += q*q;

            sum += block[k];
            Atb1 += x*block[k];
        }
    }
        
    float Atb2 = (levels-1)*sum-Atb1;
        
    float Cxx = 16*sq(levels-1)-2*(levels-1)*sum_q+sum_qq;
    float Cyy = sum_qq;
    float Cxy = (levels-1)*sum_q-sum_qq;
    float scale = (levels-1) / (Cxx*Cyy - Cxy*Cxy);

    ep[0] = (Atb1*Cyy - Atb2*Cxy)*scale;
    ep[1] = (Atb2*Cxx - Atb1*Cxy)*scale;
            
    ep[0] = clamp(ep[0], 0.f, 255.f);
    ep[1] = clamp(ep[1], 0.f, 255.f);

    if (fabs(Cxx*Cyy - Cxy*Cxy) < 0.001f)
    {
        ep[0] = sum/16;
        ep[1] = ep[0];
    }
}

float channel_opt_quant(uint qblock[2], local float* restrict block, int bits, float ep[])
{
    constant int* restrict unquant_table = get_unquant_table(bits);
    int levels = (1<<bits);

    qblock[0] = 0;
    qblock[1] = 0;

    float total_err = 0;

    for (int k=0; k<16; k++)
    {
        float proj = (block[k]-ep[0])/(ep[1]-ep[0]+0.001f);

        int q1 = (int)(proj*levels+0.5f);
        q1 = clamp(q1, 1, levels-1);

        float err0 = 0;
        float err1 = 0;
        int w0 = unquant_table[q1-1];
        int w1 = unquant_table[q1];
        
        float dec_v0 = (int)(((64-w0)*ep[0] + w0*ep[1] + 32)/64);
        float dec_v1 = (int)(((64-w1)*ep[0] + w1*ep[1] + 32)/64);
        err0 += sq(dec_v0 - block[k]);
        err1 += sq(dec_v1 - block[k]);

        int best_err = err1;
        int best_q = q1;
        if (err0<err1)
        {
            best_err = err0;
            best_q = q1-1;
        }

        qblock[k/8] += ((uint)best_q) << 4*(k%8);
        total_err += best_err;
    }

    return total_err;
}

float opt_channel(local bc7_enc_state* restrict state, constant bc7_enc_settings* restrict settings,
    uint qblock[2], int qep[2], local float* restrict block, int bits, int epbits)
{
    float ep[2] = {255,0};

    for (int k=0; k<16; k++)
    {
        ep[0] = min(ep[0], block[k]);
        ep[1] = max(ep[1], block[k]);
    }

    channel_quant_dequant(qep, ep, epbits);
    float err = channel_opt_quant(qblock, block, bits, ep);
        
    // refine
    const int refineIterations = settings->refineIterations_channel;
    for (int i=0; i<refineIterations; i++)
    {
        channel_opt_endpoints(ep, block, bits, qblock);
        channel_quant_dequant(qep, ep, epbits);
        err = channel_opt_quant(qblock, block, bits, ep);
    }

    return err;
}

void bc7_enc_mode45_candidate(local float* restrict blk, local bc7_enc_state* restrict state, constant bc7_enc_settings* restrict settings,
    mode45_parameters best_candidate[], float best_err[], int mode, int rotation, int swap)
{
    int bits = 2; 
    int abits = 2;   if (mode==4) abits = 3;
    int aepbits = 8; if (mode==4) aepbits = 6;
    if (swap==1) { bits = 3; abits = 2; } // (mode 4)

    local float* restrict block = blk + 64;
    for (int k=0; k<16; k++)
    {
        for (int p=0; p<3; p++)
            block[k+p*16] = block[k+p*16];

        if (rotation < 3)
        {
            // apply channel rotation
            if (settings->channels == 4) block[k+rotation*16] = block[k+3*16];
            if (settings->channels == 3) block[k+rotation*16] = 255;
        }
    }
    
    float ep[8];
    block_segment(ep, block, -1, 3);

    int qep[8];
    ep_quant_dequant(qep, ep, mode, 3);

    uint qblock[2];
    float err = block_quant(qblock, block, bits, ep, 0, 3);
    
    // refine
    int refineIterations = settings->refineIterations[mode];
    for (int i=0; i<refineIterations; i++)
    {
        opt_endpoints(ep, block, bits, qblock, -1, 3);
        ep_quant_dequant(qep, ep, mode, 3);
        err = block_quant(qblock, block, bits, ep, 0, 3);
    }

    // encoding selected channel 
    int aqep[2];
    uint aqblock[2];
    err += opt_channel(state, settings, aqblock, aqep, &block[rotation*16], abits, aepbits);

    if (err<*best_err)
    {
        
        swap_ints(best_candidate->qep, qep, 8);
        swap_uints(best_candidate->qblock, qblock, 2);
        swap_ints(best_candidate->aqep, aqep, 2);
        swap_uints(best_candidate->aqblock, aqblock, 2);
        best_candidate->rotation = rotation;
        best_candidate->swap = swap;
        *best_err = err;
    }	
}

void bc7_enc_mode45(local float* restrict block, local bc7_enc_state* restrict state, constant bc7_enc_settings* restrict settings)
{
    mode45_parameters best_candidate;
    float best_err = state->best_err;

    vstore16((uint16)0, 0, (uint*)&best_candidate);

    int channel0 = settings->mode45_channel0;
    for (int p=channel0; p<settings->channels; p++)
    {
        bc7_enc_mode45_candidate(block, state, settings, &best_candidate, &best_err, 4, p, 0);
        bc7_enc_mode45_candidate(block, state, settings, &best_candidate, &best_err, 4, p, 1);
    }

    // mode 4
    if (best_err<state->best_err)
    {
        state->best_err = best_err;
        bc7_code_mode45(state->best_data, &best_candidate, 4);
    }
    
    for (int p=channel0; p<settings->channels; p++)
    {
        bc7_enc_mode45_candidate(block, state, settings, &best_candidate, &best_err, 5, p, 0);
    }

    // mode 5
    if (best_err<state->best_err)
    {
        state->best_err = best_err;
        bc7_code_mode45(state->best_data, &best_candidate, 5);
    }
}

void bc7_enc_mode6(local float* restrict block, local bc7_enc_state* restrict state, constant bc7_enc_settings* restrict settings)
{
    int mode = 6;
    int bits = 4;
    float ep[8];
    block_segment(ep, block, -1, settings->channels);
    
    if (settings->channels == 3)
    {
        ep[3] = ep[7] = 255;
    }

    int qep[8];
    ep_quant_dequant(qep, ep, mode, settings->channels);

    uint qblock[2];
    float err = block_quant(qblock, block, bits, ep, 0, settings->channels);

    // refine
    int refineIterations = settings->refineIterations[mode];
    for (int i=0; i<refineIterations; i++)
    {
        opt_endpoints(ep, block, bits, qblock, -1, settings->channels);
        ep_quant_dequant(qep, ep, mode, settings->channels);
        err = block_quant(qblock, block, bits, ep, 0, settings->channels);
    }
        
    if (err<state->best_err)
    {
        state->best_err = err;
        bc7_code_mode6(state->best_data, qep, qblock);
    }
}

//////////////////////////
// BC7 bitstream coding

void bc7_code_apply_swap_mode456(int qep[], int channels, uint qblock[2], int bits)
{
    int levels = 1 << bits;
    if ((qblock[0]&15)>=levels/2)
    {
        swap_ints(&qep[0], &qep[channels], channels);
            
        for (int k=0; k<2; k++)
            qblock[k] = (uint)(0x11111111*(levels-1)) - qblock[k];
    }
}

int bc7_code_apply_swap_mode01237(int qep[], uint qblock[2], int mode, int part_id)
{
    int bits = 2;  if (mode == 0 || mode == 1) bits = 3;
    int pairs = 2; if (mode == 0 || mode == 2) pairs = 3;

    int flips = 0;
    int levels = 1 << bits;
    uchar3 skip = get_skips(part_id);
    uchar skips[3] = {skip.x, skip.y, skip.z};

    for (int j=0; j<pairs; j++)
    {
        int k0 = skips[j];
        //int q = (qblock[k0/8]>>((k0%8)*4))&15;
        int q = ((qblock[k0>>3]<<(28-(k0&7)*4))>>28);
        
        if (q>=levels/2)
        {
            swap_ints(&qep[8*j], &qep[8*j+4], 4);
            uint pmask = get_pattern_mask(part_id, j);
            flips |= pmask;
        }
    }

    return flips;
}

void put_bits(local uint* restrict data, int* restrict pos, int bits, int v)
{
    data[*pos/32] |= ((uint)v) << (*pos%32);
    if (*pos%32+bits>32)
    {
        data[*pos/32+1] |= v >> (32-*pos%32);
    }
    *pos += bits;
}

inline void data_shl_1bit_from(local uint* restrict data, int from)
{
    if (from < 96)
    {
        uint shifted = (data[2]>>1) | (data[3]<<31); 
        uint mask = (pow2(from-64)-1)>>1;
        data[2] = (mask&data[2]) | (~mask&shifted);
        data[3] = (data[3]>>1) | (data[4]<<31);
        data[4] = data[4]>>1;
    }
    else if (from < 128)
    {
        uint shifted = (data[3]>>1) | (data[4]<<31); 
        uint mask = (pow2(from-96)-1)>>1;
        data[3] = (mask&data[3]) | (~mask&shifted);
        data[4] = data[4]>>1;
    }
}

void bc7_code_qblock(local uint* restrict data, int* restrict pPos, uint qblock[2], int bits, int flips)
{
    int levels = 1 << bits;
    int flips_shifted = flips;
    for (int k1=0; k1<2; k1++)
    {
        uint qbits_shifted = qblock[k1];
        for (int k2=0; k2<8; k2++)
        {
            int q = qbits_shifted&15;
            if ((flips_shifted&1)>0) q = (levels-1)-q;

            if (k1==0 && k2==0)	put_bits(data, pPos, bits-1, q);
            else				put_bits(data, pPos, bits  , q);
            qbits_shifted >>= 4;
            flips_shifted >>= 1;
        }
    }
}

void bc7_code_adjust_skip_mode01237(local uint* restrict data, int mode, int part_id)
{
    int bits = 2;  if (mode == 0 || mode == 1) bits = 3;
    int pairs = 2; if (mode == 0 || mode == 2) pairs = 3;
        
    uchar3 skip = get_skips(part_id);
    if (pairs>2 && skip.y < skip.z)
    {
        int t = skip.y; skip.y = skip.z; skip.z = t;
    }
    uchar skips[3] = {skip.x, skip.y, skip.z};

    for (int j=1; j<pairs; j++)
    {
        int k = skips[j];
        data_shl_1bit_from(data, 128+(pairs-1)-(15-k)*bits);
    }
}

void bc7_code_mode01237(local uint* restrict data, int* restrict qep, uint qblock[2], int part_id, int mode)
{
    int bits = 2;  if (mode == 0 || mode == 1) bits = 3;
    int pairs = 2; if (mode == 0 || mode == 2) pairs = 3;
    int channels = 3; if (mode == 7) channels = 4;

    int flips = bc7_code_apply_swap_mode01237(qep, qblock, mode, part_id);

    for (int k=0; k<5; k++) data[k] = 0;
    int pos = 0;

    // mode 0-3, 7
    put_bits(data, &pos, mode+1, 1<<mode);
    
    // partition
    if (mode==0)
    {
        put_bits(data, &pos, 4, part_id&15);
    }
    else
    {
        put_bits(data, &pos, 6, part_id&63);
    }
    
    // endpoints
    for (int p=0; p<channels; p++)
    for (int j=0; j<pairs*2; j++)
    {
        if (mode == 0)
        {
            put_bits(data, &pos, 4, qep[j*4+0+p]>>1);
        }
        else if (mode == 1)
        {
            put_bits(data, &pos, 6, qep[j*4+0+p]>>1);
        }
        else if (mode == 2)
        {
            put_bits(data, &pos, 5, qep[j*4+0+p]);
        }
        else if (mode == 3)
        {
            put_bits(data, &pos, 7, qep[j*4+0+p]>>1);
        }
        else if (mode == 7)
        {
            put_bits(data, &pos, 5, qep[j*4+0+p]>>1);
        }
    }
    
    // p bits
    if (mode == 1)
    for (int j=0; j<2; j++)
    {
        put_bits(data, &pos, 1, qep[j*8]&1);
    }
    
    if (mode == 0 || mode == 3 || mode == 7)
    for (int j=0; j<pairs*2; j++)
    {
        put_bits(data, &pos, 1, qep[j*4]&1);
    }
    
    // quantized values
    bc7_code_qblock(data, &pos, qblock, bits, flips);
    bc7_code_adjust_skip_mode01237(data, mode, part_id);
}

void bc7_code_mode45(local uint* restrict data, mode45_parameters* restrict params, int mode)
{
    int qep[8];
    uint qblock[2];
    int aqep[2];
    uint aqblock[2];

    swap_ints(params->qep, qep, 8);
    swap_uints(params->qblock, qblock, 2);
    swap_ints(params->aqep, aqep, 2);
    swap_uints(params->aqblock, aqblock, 2);
    int rotation = params->rotation;
    int swap = params->swap;	
    
    int bits = 2; 
    int abits = 2;   if (mode==4) abits = 3;
    int epbits = 7;  if (mode==4) epbits = 5;
    int aepbits = 8; if (mode==4) aepbits = 6;

    if (!swap)
    {
        bc7_code_apply_swap_mode456(qep, 4, qblock, bits);
        bc7_code_apply_swap_mode456(aqep, 1, aqblock, abits);
    }
    else
    {
        swap_uints(qblock, aqblock, 2);
        bc7_code_apply_swap_mode456(aqep, 1, qblock, bits);
        bc7_code_apply_swap_mode456(qep, 4, aqblock, abits);
    }

    for (int k=0; k<5; k++) data[k] = 0;
    int pos = 0;
    
    // mode 4-5
    put_bits(data, &pos, mode+1, 1<<mode);
    
    // rotation
    //put_bits(data, &pos, 2, (rotation+1)%4);
    put_bits(data, &pos, 2, (rotation+1)&3);
    
    if (mode==4)
    {
        put_bits(data, &pos, 1, swap);
    }
    
    // endpoints
    for (int p=0; p<3; p++)
    {
        put_bits(data, &pos, epbits, qep[0+p]);
        put_bits(data, &pos, epbits, qep[4+p]);
    }
    
    // alpha endpoints
    put_bits(data, &pos, aepbits, aqep[0]);
    put_bits(data, &pos, aepbits, aqep[1]);
        
    // quantized values
    bc7_code_qblock(data, &pos, qblock, bits, 0);
    bc7_code_qblock(data, &pos, aqblock, abits, 0);
}

void bc7_code_mode6(local uint* restrict data, int qep[8], uint qblock[2])
{
    bc7_code_apply_swap_mode456(qep, 4, qblock, 4);

    for (int k=0; k<5; k++) data[k] = 0;
    int pos = 0;

    // mode 6
    put_bits(data, &pos, 7, 64);
    
    // endpoints
    for (int p=0; p<4; p++)
    {
        put_bits(data, &pos, 7, qep[0+p]>>1);
        put_bits(data, &pos, 7, qep[4+p]>>1);
    }
    
    // p bits
    put_bits(data, &pos, 1, qep[0]&1);
    put_bits(data, &pos, 1, qep[4]&1);
    
    // quantized values
    bc7_code_qblock(data, &pos, qblock, 4, 0);
}


//////////////////////////
//       BC7 core

//load_block_interleaved_rgba and compute_opaque_err 
inline float load_block_interleaved_rgbaEX(local float* restrict block, global const uchar* restrict source,
    const uint stride, const uint channel)
{
    float err = 0.f;
    for (uint i = 0; i < 4; ++i)
    {
        const uchar16 color = vload16(stride * i, source);
        vstore4(convert_float4(color.s048c), i*4+0, block);
        vstore4(convert_float4(color.s159d), i*4+1, block);
        vstore4(convert_float4(color.s26ae), i*4+2, block);
        float4 alpha = convert_float4(color.s37bf);
        vstore4(alpha, i*4+3, block);
        alpha -= 255.f;
        alpha = alpha*alpha;
        err += alpha.x + alpha.y + alpha.z + alpha.w;
    }
    return err;
}

kernel void CompressBlockBC7(global const uchar* restrict src, global uchar* restrict dst, 
    local float* restrict blk, local bc7_enc_state* restrict st, constant bc7_enc_settings* restrict settings)
{
    private const ushort xx = get_global_id(0), yy = get_global_id(1);
    private const uchar lidX = get_local_id(0), lidY = get_local_id(1), lid = lidY * get_local_size(0) + lidX;
    local float* restrict block = blk + lid * (64 + 48);
    local bc7_enc_state* restrict state = st + lid;
    if (xx * 4 < settings->width && yy * 4 < settings->height)
    {
        state->opaque_err = load_block_interleaved_rgbaEX(block, src + yy*4*settings->stride + xx*4*4, settings->stride, settings->channels);
        state->best_err = 1e30f;

        if (settings->mode_selection[0]) bc7_enc_mode02(block, state, settings);
        if (settings->mode_selection[1]) bc7_enc_mode13(block, state, settings);
        if (settings->mode_selection[1]) bc7_enc_mode7 (block, state, settings);
        if (settings->mode_selection[2]) bc7_enc_mode45(block, state, settings);
        if (settings->mode_selection[3]) bc7_enc_mode6 (block, state, settings);

        vstore4(vload4(0, state->best_data), 0, (global uint*)(dst + yy*settings->width*16 + xx*16));
    }
}
