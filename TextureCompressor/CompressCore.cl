#define uint8_t uchar
#define uint16_t ushort
#define uint32_t uint

struct bc7_enc_settings
{
    bool mode_selection[4];
    int refineIterations[8];

    bool skip_mode2;
    int fastSkipTreshold_mode1;
    int fastSkipTreshold_mode3;
    int fastSkipTreshold_mode7;

    int mode45_channel0;
    int refineIterations_channel;
};


inline void load_block_interleaved_rgba(local float * const restrict block, global const uchar4* const restrict src, const uint stride)
{
#pragma unroll
    for (uint y = 0; y < 4; y++)
        for (uint x = 0; x < 4; x++)
        {
            const uchar4 rgba = src[y * stride + x];
            const float4 rgbaf = convert_float4(rgba);
            block[16 * 0 + y * 4 + x] = rgbaf.r;
            block[16 * 1 + y * 4 + x] = rgbaf.g;
            block[16 * 2 + y * 4 + x] = rgbaf.b;
            block[16 * 3 + y * 4 + x] = rgbaf.a;
        }
}

inline float compute_opaque_err(local float * const restrict block, const int channels)
{
    if (channels == 3) 
        return 0.0f;
    float err = 0.0f;
    for (uint k = 0; k < 16; k++)
    {
        const float v = block[48 + k] - 255.0f;
        err += v * v;
    }
    return err;
}


float bc7_enc_mode01237_part_fast(int qep[24], uint32 qblock[2], float block[64], int part_id, uniform int mode)
{
    uint32 pattern = get_pattern(part_id);
    uniform int bits = 2;  if (mode == 0 || mode == 1) bits = 3;
    uniform int pairs = 2; if (mode == 0 || mode == 2) pairs = 3;
    uniform int channels = 3; if (mode == 7) channels = 4;

    float ep[24];
    for (uniform int j = 0; j<pairs; j++)
    {
        int mask = get_pattern_mask(part_id, j);
        block_segment(&ep[j * 8], block, mask, channels);
    }

    ep_quant_dequant(qep, ep, mode, channels);

    float total_err = block_quant(qblock, block, bits, ep, pattern, channels);
    return total_err;
}

void bc7_enc_mode01237(bc7_enc_state state[], uniform int mode, int part_list[], uniform int part_count)
{
    if (part_count == 0) return;
    uniform int bits = 2;  if (mode == 0 || mode == 1) bits = 3;
    uniform int pairs = 2; if (mode == 0 || mode == 2) pairs = 3;
    uniform int channels = 3; if (mode == 7) channels = 4;

    int best_qep[24];
    uint32 best_qblock[2];
    int best_part_id = -1;
    float best_err = 1e99;

    for (uniform int part = 0; part<part_count; part++)
    {
        int part_id = part_list[part] & 63;
        if (pairs == 3) part_id += 64;

        int qep[24];
        uint32 qblock[2];
        float err = bc7_enc_mode01237_part_fast(qep, qblock, state->block, part_id, mode);

        if (err<best_err)
        {
            for (uniform int i = 0; i<8 * pairs; i++) best_qep[i] = qep[i];
            for (uniform int k = 0; k<2; k++) best_qblock[k] = qblock[k];
            best_part_id = part_id;
            best_err = err;
        }
    }

    // refine
    uniform int refineIterations = state->refineIterations[mode];
    for (uniform int _ = 0; _<refineIterations; _++)
    {
        float ep[24];
        for (uniform int j = 0; j<pairs; j++)
        {
            int mask = get_pattern_mask(best_part_id, j);
            opt_endpoints(&ep[j * 8], state->block, bits, best_qblock, mask, channels);
        }

        int qep[24];
        uint32 qblock[2];

        ep_quant_dequant(qep, ep, mode, channels);

        uint32 pattern = get_pattern(best_part_id);
        float err = block_quant(qblock, state->block, bits, ep, pattern, channels);

        if (err<best_err)
        {
            for (uniform int i = 0; i<8 * pairs; i++) best_qep[i] = qep[i];
            for (uniform int k = 0; k<2; k++) best_qblock[k] = qblock[k];
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

inline void bc7_enc_mode02(constant bc7_enc_settings* settings, private float* best_err)
{
    int part_list[64];
    for (int part = 0; part < 64; part++)
        part_list[part] = part;

    bc7_enc_mode01237(settings, 0, part_list, 16);
    if (!settings->skip_mode2) 
        bc7_enc_mode01237(settings, 2, part_list, 64); // usually not worth the time
}

inline void bc7_enc_mode13(constant bc7_enc_settings* settings, private float* best_err)
{
    if (state->fastSkipTreshold_mode1 == 0 && state->fastSkipTreshold_mode3 == 0) return;

    float full_stats[15];
    compute_stats_masked(full_stats, state->block, -1, 3);

    int part_list[64];
    for (uniform int part = 0; part<64; part++)
    {
        int mask = get_pattern_mask(part + 0, 0);
        float bound12 = block_pca_bound_split(state->block, mask, full_stats, 3);
        int bound = (int)(bound12);
        part_list[part] = part + bound * 64;
    }

    partial_sort_list(part_list, 64, max(state->fastSkipTreshold_mode1, state->fastSkipTreshold_mode3));
    bc7_enc_mode01237(state, 1, part_list, state->fastSkipTreshold_mode1);
    bc7_enc_mode01237(state, 3, part_list, state->fastSkipTreshold_mode3);
}

inline void bc7_enc_mode7(constant bc7_enc_settings* settings, private float* best_err)
{
    if (state->fastSkipTreshold_mode7 == 0) return;

    float full_stats[15];
    compute_stats_masked(full_stats, state->block, -1, state->channels);

    int part_list[64];
    for (uniform int part = 0; part<64; part++)
    {
        int mask = get_pattern_mask(part + 0, 0);
        float bound12 = block_pca_bound_split(state->block, mask, full_stats, state->channels);
        int bound = (int)(bound12);
        part_list[part] = part + bound * 64;
    }

    partial_sort_list(part_list, 64, state->fastSkipTreshold_mode7);
    bc7_enc_mode01237(state, 7, part_list, state->fastSkipTreshold_mode7);
}

kernel void CompressBlocksBC7(global const uchar4* restrict src, global uint8 * const restrict dst, constant bc7_enc_settings settings, constant uint width, constant uint height, constant uint stride)
{
    //for (uniform int yy = 0; yy<src->height / 4; yy++)
    //    foreach(xx = 0 ... src->width / 4)
    //{
    //    CompressBlockBC7(src, xx, yy, dst, settings);
    //}
    private const uint tId = get_local_id(0);
    private const uint blkId = get_group_id(0) * get_local_size(0) + tId;
    private const uint xx = blkId % (width / 4), yy = blkId / (width / 4);
    
    local float block[65 * 128]; //shared local block
    private float opaque_err;       // error for coding alpha=255
    private float best_err = 1e99;
    private uint best_data[5];	// 4, +1 margin for skips
    local float* const restrict myBlock = block + 65 * tId;
    
    src = src + (width * yy) + xx;
    
    load_block_interleaved_rgba(myBlock, src, stride);
    
    opaque_err = compute_opaque_err(myBlock, 4); // force using RGBA

    if (settings.mode_selection[0]) 
        bc7_enc_mode02(&settings, &best_err);
    if (settings.mode_selection[1]) 
        bc7_enc_mode13(&settings, &best_err);
    if (settings.mode_selection[1])
        bc7_enc_mode7(&settings, &best_err);
    if (settings.mode_selection[2]) 
        bc7_enc_mode45(&settings, &best_err);
    if (settings.mode_selection[3])
        bc7_enc_mode6(&settings, &best_err);

    store_data(dst, width, xx, yy, best_data, 4);

}
