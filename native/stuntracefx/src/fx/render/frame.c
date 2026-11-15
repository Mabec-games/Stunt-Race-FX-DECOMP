/*
 * frame.c: the Super FX code of asm/stuntracefx/render/frame.asm, written from it by
 * the generator.
 *
 * frame.asm: the Super FX's frame program (01:AB13), the object walk
 * (01:AB81), the object sprite (01:B0D9), the effects' spawn (01:B6D5)
 * and the weather (01:BC7D). C: src/stuntracefx/render/frame.c.
 */

#include "fx/fx.h"
#include "fx/fx_far.h"
#include "fx/fx_draw.h"
#include "fx/fx_ground.h"
#include "fx/fx_view.h"
#include "fx/fx_weather.h"
#include "nshvc/nshvc.h"
#include "system/perf.h"
#include "video/video.h"

/*
 * @routine 01:AB13 FxFrameProgram
 * WHAT IT DOES: A program the S-CPU starts once a frame: clears the
 * screen (FxScreenClear, unless the byte at $01C4 is 0), draws the
 * objects of the sorted draw list (FxObjectsDraw), the weather
 * (FxWeatherDraw, by bits 0 and 1 of the word at +$8A of the record
 * $247A points to; none clears $2B94), the effects (FxEffectsRun) and
 * the text (FxTextNotice), then STOP.
 * HOW: R10 = $0240 before each part; $01B4 = 0; CMODE 0; $26EE = $10
 * for the first weather; RPIX before the STOP writes the pixel caches
 * out.
 * Entry: started by the S-CPU.
 * Exit: (at the STOP) as the last part leaves the registers, flags and
 * RAM; R14 and the ROM buffer move as on the Super FX.
 */
static FxFrameProgramResult frame_program(uint16_t draw_list_end);

/* The frame program's drawing is the picture's 3D; the HUD's programs that
   follow it draw the HUD (fx_draw_part). */
FxFrameProgramResult FxFrameProgram(uint16_t draw_list_end)
{
    FxFrameProgramResult r;

    fx_draw_part(0);
    r = frame_program(draw_list_end);
    fx_draw_part(1);
    return r;
}

static FxFrameProgramResult frame_program(uint16_t draw_list_end)
{
    uint16_t stack_top;
    uint16_t *stack = &stack_top;
    uint16_t weather;
    uint16_t word_1c4;
    uint16_t notice_car;
    uint16_t weather_flags;
    uint16_t weather_record;
    uint16_t last_plot_result;
    uint16_t text_draw_result;
    uint16_t weather_record_ptr;

    (void)draw_list_end;                /* the near list's end: the pass walks the list itself (fx_far.c) */
    word_1c4 = fx_ldw(GSU_WORD_1C4);
    if ((uint16_t)(word_1c4 & 0xFF) == 0) {
    } else {
        FxScreenClear();
    }
    *stack = 0x0240;
    fx_stw(FX_MODEL_1B4, 0);
    if (race_full_window() || race_two_full_window()) {
        nshvc_profile_begin(perf_ground);
        fx_ground_draw();               /* the course's ground or water under everything (fx_ground.c) */
        nshvc_profile_end();
    }
    nshvc_profile_begin(perf_far);
    fx_far_pass();                      /* the objects past the near list's block, in view (fx_far.c) */
    nshvc_profile_end();
    *stack = 0x0240;
    nshvc_profile_begin(perf_near);
    fx_far_parked(stack);               /* every object, near and far, in one order by depth (fx_far.c) */
    nshvc_profile_end();
    fx_draw_held_by_view(0);
    fx_draw_water();                    /* with the depth test the see-through water after the solid 3D (fx_draw.c) */
    *stack = 0x0240;
    fx_cmode(0);
    weather_record = fx_ldw(0x247A);
    weather_record_ptr = weather_record;
    if (weather_record == 0) {
        fx_stw(FX_INIT_2B94, 0);
    } else {
        weather_flags = fx_ldw((uint16_t)(weather_record_ptr + 0x008A));
        if ((1 & weather_flags) == 0) {
            if ((2 & weather_flags) == 0) {
                fx_stw(FX_INIT_2B94, 0);
            } else {
                weather = 2;
                weather_record_ptr = FxWeatherDraw(weather, 0x0050, weather_record_ptr, stack);
            }
        } else {
            fx_stw(FX_EFFECT_MAX, 0x0010);
            weather = 1;
            weather_record_ptr = FxWeatherDraw(weather, 0x0050, weather_record_ptr, stack);
        }
    }
    last_plot_result = FxEffectsRun(weather_record_ptr, stack);
    fx_draw_world_end();                /* the text is on the screen, not in the world (fx_draw.c) */
    fx_stw(FX_EFFECT_MAX, 0);
    { FxTextNoticeResult r_ = FxTextNotice(last_plot_result, stack); text_draw_result = r_.text_draw_result; notice_car = r_.notice_car; };
    fx_rpix((uint8_t)text_draw_result, (uint8_t)notice_car);
    return (FxFrameProgramResult){0};
}


/*
 * @routine 01:AB81 FxObjectsDraw
 * WHAT IT DOES: Draws every object in the draw list from $023C: its
 * angles, flags and shape from the slot; a slot whose flags have bit 2
 * adds a burst of effect particles (FxEffectsAdd) and draws them
 * (FxParticlesDraw); else the shape's header (in ROM bank $14) picks
 * the model by distance (+$16, +$18, +$1A at depths $3E8, $7D0, $BB8)
 * and the object is drawn as a sprite (FxObjectSprite) or a model
 * (FxModelDraw).
 * HOW: Pushes R11 on the R10 stack and each slot's address around its
 * drawing; the next slot from its link word (+$00) until 0; returns by
 * pulling R15.
 * Entry: R10 = stack; R11 = return address.
 * Exit: as the last object leaves the registers, flags and RAM; R14 and
 * the ROM buffer move as on the Super FX.
 */
FxObjectsDrawResult FxObjectsDraw(uint16_t draw_list_end, uint16_t *stack)
{
    int up_ = 0;                       /* levels a callee returned past us */
    uint16_t draw_list;
    uint16_t flags_ptr;
    uint16_t model_148;
    uint16_t model_14e;
    uint16_t model_150;
    uint16_t model_1b4;
    uint16_t model_1b6;
    uint16_t pos_x_ptr;
    uint16_t pos_y_ptr;
    uint16_t pos_z_ptr;
    uint16_t tex_u_ptr;
    uint16_t draw_flags;
    uint16_t model_bank;
    uint16_t object_ptr;
    uint16_t param_108a;
    uint16_t points_ptr;
    uint16_t angle_x_ptr;
    uint16_t angle_y_ptr;
    uint16_t angle_z_ptr;
    uint16_t lod_far_low;
    uint16_t lod_mid_low;
    uint16_t model_148_2;
    uint16_t model_148_3;
    uint16_t object_kind;
    uint16_t sprite_size;
    uint16_t angle_x_byte;
    uint16_t angle_y_byte;
    uint16_t angle_z_byte;
    uint16_t effect_count;
    uint16_t field_44_ptr;
    uint16_t geometry_ptr;
    uint16_t lod_near_low;
    uint16_t object_pos_x;
    uint16_t object_pos_y;
    uint16_t object_pos_z;
    uint16_t lod_depth;
    uint16_t points_total;
    uint16_t tex_u_offset;
    uint16_t tex_v_offset;
    uint16_t lod_model_ptr;
    uint16_t material_mask;
    uint16_t model_1b6_ptr;
    uint16_t effect_rec_ptr;
    uint16_t material_table;
    uint16_t points_ptr_low;
    uint16_t points_ptr_ptr;
    uint16_t points_total_2;
    uint16_t effect_byte_14e;
    uint16_t effect_byte_150;
    uint16_t lod_far_ptr_ptr;
    uint16_t lod_mid_ptr_ptr;
    uint16_t model_field_ptr;
    uint16_t next_object_ptr;
    uint16_t object_field_44;
    uint16_t object_kind_ptr;
    uint16_t sprite_material;
    uint16_t sprite_offset_y;
    uint16_t sprite_size_low;
    uint16_t sprite_size_ptr;
    uint16_t geometry_ptr_low;
    uint16_t lod_near_ptr_ptr;
    uint16_t model_header_ptr;
    uint16_t default_table_low;
    uint16_t default_table_ptr;
    uint16_t effect_count_byte;
    uint16_t material_mask_ptr;
    uint16_t model_detail_byte;
    uint16_t popped_object_ptr;
    uint16_t model_points_check;
    uint16_t restored_model_ptr;
    uint16_t selected_model_ptr;
    uint16_t table_override_ptr;
    uint16_t effect_byte_14e_ptr;
    uint16_t effect_byte_150_ptr;
    uint16_t sprite_geometry_ptr;
    uint16_t table_override_word;

    draw_list = fx_ldw(GSU_DRAW_LIST);
    object_ptr = draw_list;
    if (draw_list != 0) {
        fx_spill(stack);
        for (;;) {
            fx_romb(0);
            fx_cmode(0);
            fx_stw(FX_POINTS_TOTAL, 0);
            fx_push(stack, object_ptr);
            angle_z_ptr = (uint16_t)(object_ptr + 4);
            angle_z_byte = fx_ldb(angle_z_ptr);
            fx_stw(0x0020, angle_z_byte);
            angle_x_ptr = (uint16_t)(angle_z_ptr + 1);
            angle_x_byte = fx_ldb(angle_x_ptr);
            fx_stw(0x0022, angle_x_byte);
            angle_y_ptr = (uint16_t)(angle_x_ptr + 1);
            angle_y_byte = fx_ldb(angle_y_ptr);
            fx_stw(0x0024, angle_y_byte);
            fx_angle_record(object_ptr);
            fx_far_record(object_ptr);
            fx_face_object(object_ptr);         /* its corners made exactly from its draw record (fx_face.c) */
            flags_ptr = (uint16_t)(angle_y_ptr + 1);
            draw_flags = fx_ldb(flags_ptr);
            fx_draw_held_by_view(draw_flags & 0x80);     /* placed in the view's space: not the world's (fx_draw.c) */
            model_field_ptr = (uint16_t)(flags_ptr + 1);
            model_header_ptr = fx_ldw(model_field_ptr);
            fx_solid_object(model_header_ptr, object_ptr);
            fx_face_shape_now(model_header_ptr);
            selected_model_ptr = model_header_ptr;
            if (model_header_ptr != 0) {
                pos_y_ptr = (uint16_t)(model_field_ptr + 8);
                object_pos_y = fx_ldw(pos_y_ptr);
                fx_stw(0x0028, object_pos_y);
                pos_x_ptr = (uint16_t)(pos_y_ptr + 2);
                object_pos_x = fx_ldw(pos_x_ptr);
                fx_stw(0x0026, object_pos_x);
                pos_z_ptr = (uint16_t)(pos_x_ptr + 2);
                object_pos_z = fx_ldw(pos_z_ptr);
                fx_stw(0x002A, object_pos_z);
                table_override_ptr = (uint16_t)(pos_z_ptr + 2);
                if ((draw_flags & 4) == 0) {
                    if ((draw_flags & 2) == 0) {
                        table_override_word = fx_ldw(table_override_ptr);
                        material_table = table_override_word;
                        if (table_override_word != 0) {
                        } else {
                            fx_r14(0x0012);
                            default_table_ptr = (uint16_t)(selected_model_ptr + 0x0012);
                            fx_r14(default_table_ptr);
                            default_table_low = fx_getb();
                            fx_r14((uint16_t)(default_table_ptr + 1));
                            material_table = (uint16_t)((default_table_low & 0xFF) | fx_getb() << 8);
                            if ((uint16_t)(material_table + 0x706F) != 0) {
                            } else {
                                param_108a = fx_ldw(GSU_PARAM_108A);
                                if (param_108a == 0) {
                                } else {
                                    material_table = param_108a;
                                }
                            }
                        }
                        fx_stw(0x004A, material_table);
                    } else {
                        if ((draw_flags & 8) != 0) {
                            table_override_word = fx_ldw(table_override_ptr);
                            material_table = table_override_word;
                            if (table_override_word != 0) {
                            } else {
                                fx_r14(0x0012);
                                default_table_ptr = (uint16_t)(selected_model_ptr + 0x0012);
                                fx_r14(default_table_ptr);
                                default_table_low = fx_getb();
                                fx_r14((uint16_t)(default_table_ptr + 1));
                                material_table = (uint16_t)((default_table_low & 0xFF) | fx_getb() << 8);
                                if ((uint16_t)(material_table + 0x706F) != 0) {
                                } else {
                                    param_108a = fx_ldw(GSU_PARAM_108A);
                                    if (param_108a == 0) {
                                    } else {
                                        material_table = param_108a;
                                    }
                                }
                            }
                            fx_stw(0x004A, material_table);
                        } else {
                            fx_stw(0x004A, 0x8D30);
                        }
                    }
                    object_kind_ptr = (uint16_t)(table_override_ptr + 2);
                    fx_r14(7);
                    fx_r14((uint16_t)(selected_model_ptr + 7));
                    model_detail_byte = fx_getb();
                    fx_stw(0x0032, model_detail_byte);
                    fx_push(stack, selected_model_ptr);
                    lod_depth = fx_far_depth(object_pos_z);     /* a far object's by its true depth */
                    if (lod_depth >= 0x0BB8) {
                        fx_r14(0x001A);
                        lod_far_ptr_ptr = (uint16_t)(selected_model_ptr + 0x001A);
                        fx_r14(lod_far_ptr_ptr);
                        lod_far_low = fx_getb();
                        fx_r14((uint16_t)(lod_far_ptr_ptr + 1));
                        selected_model_ptr = (uint16_t)((lod_far_low & 0xFF) | fx_getb() << 8);
                    } else {
                        if (lod_depth >= 0x07D0) {
                            fx_r14(0x0018);
                            lod_mid_ptr_ptr = (uint16_t)(selected_model_ptr + 0x0018);
                            fx_r14(lod_mid_ptr_ptr);
                            lod_mid_low = fx_getb();
                            fx_r14((uint16_t)(lod_mid_ptr_ptr + 1));
                            selected_model_ptr = (uint16_t)((lod_mid_low & 0xFF) | fx_getb() << 8);
                        } else {
                            if (lod_depth >= 0x03E8) {
                                fx_r14(0x0016);
                                lod_near_ptr_ptr = (uint16_t)(selected_model_ptr + 0x0016);
                                fx_r14(lod_near_ptr_ptr);
                                lod_near_low = fx_getb();
                                fx_r14((uint16_t)(lod_near_ptr_ptr + 1));
                                selected_model_ptr = (uint16_t)((lod_near_low & 0xFF) | fx_getb() << 8);
                            } else {
                            }
                        }
                    }
                    fx_r14(0);
                    lod_model_ptr = selected_model_ptr;
                    fx_r14(lod_model_ptr);
                    geometry_ptr_low = fx_getb();
                    fx_r14((uint16_t)(lod_model_ptr + 1));
                    geometry_ptr = (uint16_t)((geometry_ptr_low & 0xFF) | fx_getb() << 8);
                    fx_stw(0x0016, geometry_ptr);
                    fx_r14(2);
                    fx_r14((uint16_t)(selected_model_ptr + 2));
                    model_bank = fx_getb();
                    fx_stw(0x001C, model_bank);
                    fx_r14(3);
                    points_ptr_ptr = (uint16_t)(selected_model_ptr + 3);
                    fx_r14(points_ptr_ptr);
                    points_ptr_low = fx_getb();
                    fx_r14((uint16_t)(points_ptr_ptr + 1));
                    points_ptr = (uint16_t)((points_ptr_low & 0xFF) | fx_getb() << 8);
                    fx_stw(0x0018, points_ptr);
                    restored_model_ptr = fx_pull(stack);
                    object_kind = fx_ldb(object_kind_ptr);
                    field_44_ptr = (uint16_t)(object_kind_ptr + 1);
                    fx_stw(FX_OBJECT_KIND, object_kind);
                    object_field_44 = fx_ldb(field_44_ptr);
                    fx_stw(0x0044, object_field_44);
                    material_mask_ptr = (uint16_t)(field_44_ptr + 1);
                    material_mask = fx_ldb(material_mask_ptr);
                    fx_stw(FX_MATERIAL_MASK, material_mask);
                    model_1b6_ptr = (uint16_t)(material_mask_ptr + 1);
                    if ((draw_flags & 8) == 0) {
                        model_1b6 = fx_ldb(model_1b6_ptr);
                        fx_stw(FX_MODEL_1B6, model_1b6);
                        tex_u_ptr = (uint16_t)(model_1b6_ptr + 1);
                        tex_u_offset = fx_ldb(tex_u_ptr);
                        fx_stw(FX_TEX_U_OFF, (uint16_t)(tex_u_offset << 8 | tex_u_offset >> 8));
                        tex_v_offset = fx_ldb((uint16_t)(tex_u_ptr + 1));
                        fx_stw(FX_TEX_V_OFF, (uint16_t)(tex_v_offset << 8 | tex_v_offset >> 8));
                        model_points_check = fx_ldw(0x0018);
                        if (model_points_check == 0) {
                        } else {
                            { FxModelDrawResult r_ = FxModelDraw(draw_list_end, stack); up_ = r_.up; draw_list_end = r_.draw_list_end_out; };
                            if (up_) {                       /* it returned past us */
                                return (FxObjectsDrawResult){up_ == 2 ? 0 : up_ - 2};
                            }
                        }
                    } else {
                        model_1b4 = fx_ldw(FX_MODEL_1B4);
                        if ((uint16_t)(model_1b4 & 0xFF) != 0) {
                            model_1b6 = fx_ldb(model_1b6_ptr);
                            fx_stw(FX_MODEL_1B6, model_1b6);
                            tex_u_ptr = (uint16_t)(model_1b6_ptr + 1);
                            tex_u_offset = fx_ldb(tex_u_ptr);
                            fx_stw(FX_TEX_U_OFF, (uint16_t)(tex_u_offset << 8 | tex_u_offset >> 8));
                            tex_v_offset = fx_ldb((uint16_t)(tex_u_ptr + 1));
                            fx_stw(FX_TEX_V_OFF, (uint16_t)(tex_v_offset << 8 | tex_v_offset >> 8));
                            model_points_check = fx_ldw(0x0018);
                            if (model_points_check == 0) {
                            } else {
                                { FxModelDrawResult r_ = FxModelDraw(draw_list_end, stack); up_ = r_.up; draw_list_end = r_.draw_list_end_out; };
                                if (up_) {                       /* it returned past us */
                                    return (FxObjectsDrawResult){up_ == 2 ? 0 : up_ - 2};
                                }
                            }
                        } else {
                            fx_romb(0);
                            fx_r14(0x0010);
                            sprite_size_ptr = (uint16_t)(restored_model_ptr + 0x0010);
                            fx_r14(sprite_size_ptr);
                            sprite_size_low = fx_getb();
                            fx_r14((uint16_t)(sprite_size_ptr + 1));
                            sprite_size = (uint16_t)((sprite_size_low & 0xFF) | fx_getb() << 8);
                            fx_stw(FX_SPRITE_SIZE, sprite_size);
                            sprite_material = fx_ldb(model_1b6_ptr);
                            fx_stw(FX_SPRITE_MATERIAL, sprite_material);
                            sprite_offset_y = fx_ldb((uint16_t)(model_1b6_ptr + 1));
                            fx_stw(FX_MODEL_CE, (uint16_t)(int16_t)(int8_t)sprite_offset_y);
                            sprite_geometry_ptr = fx_ldw(0x0016);
                            if (sprite_geometry_ptr == 0) {
                            } else {
                                { FxObjectSpriteResult r_ = FxObjectSprite(8, draw_list_end, stack); up_ = r_.up; draw_list_end = r_.source_row_accum_out; };
                                if (up_) {                       /* it returned past us */
                                    return (FxObjectsDrawResult){up_ == 2 ? 0 : up_ - 2};
                                }
                            }
                        }
                    }
                } else {
                    effect_rec_ptr = (uint16_t)(table_override_ptr - 0x000C);
                    model_148 = fx_ldw(effect_rec_ptr);
                    fx_stw(FX_MODEL_148, model_148);
                    effect_byte_14e_ptr = (uint16_t)(effect_rec_ptr + 2);
                    effect_byte_14e = fx_ldb(effect_byte_14e_ptr);
                    fx_stw(FX_MODEL_14E, (uint16_t)(effect_byte_14e & 0xFF));
                    effect_byte_150_ptr = (uint16_t)(effect_byte_14e_ptr + 1);
                    effect_byte_150 = fx_ldb(effect_byte_150_ptr);
                    fx_stw(FX_MODEL_150, (uint16_t)(effect_byte_150 & 0xFF));
                    effect_count_byte = fx_ldb((uint16_t)(effect_byte_150_ptr + 1));
                    effect_count = ((uint16_t)(effect_count_byte & 0xFF) & 7);
                    points_total = effect_count;
                    if (effect_count != 0) {
                    } else {
                        points_total = 0xFFFF;
                    }
                    fx_stw(FX_POINTS_TOTAL, points_total);
                    points_total_2 = fx_ldw(FX_POINTS_TOTAL);
                    if (points_total_2 == 0) {
                    } else {
                        if (points_total_2 & 0x8000) {
                        } else {
                            model_150 = fx_ldw(FX_MODEL_150);
                            model_148_2 = fx_ldw(FX_MODEL_148);
                            model_14e = fx_ldw(FX_MODEL_14E);
                            FxEffectsAdd(points_total_2, model_150, model_148_2, model_14e, stack);
                        }
                        model_148_3 = fx_ldw(FX_MODEL_148);
                        draw_list_end = FxParticlesDraw(model_148_3, stack);
                    }
                }
            } else {
            }
            fx_face_object(0);                  /* no object's: what follows draws from the chip's numbers */
            popped_object_ptr = fx_pull(stack);
            next_object_ptr = fx_ldw(popped_object_ptr);
            object_ptr = next_object_ptr;
            if (next_object_ptr == 0) {
                break;
            } else {
                continue;
            }
        }
        fx_unspill(stack);
        return (FxObjectsDrawResult){0};
    } else {
        return (FxObjectsDrawResult){0};
    }
}


/*
 * @routine 01:B0D9 FxObjectSprite
 * WHAT IT DOES: Draws an object as a sprite scaled by its depth: its
 * screen position (FxProjectScaled), its colour from its material
 * (FxSpriteMaterial), and its rows of texels plotted, clipped to the
 * screen; nothing when its depth $002A is at most $0080.
 * HOW: Pushes R11 on the R10 stack; the size from $00CE and $00AA
 * shifted by $0032; the drawing goes on in FxObjectSpriteRows
 * (01:B2ED); returns by pulling R15.
 * Entry: R10 = stack; R11 = return address; the object in $0020-$002A,
 * $00AA, $00CE, $00D6.
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
static FxObjectSpriteResult object_sprite(uint16_t sprite_scale, uint16_t draw_list_end, uint16_t *stack);

/* The sprite, or a solid in its place (fx_solid.c), drawn as it returns. */
FxObjectSpriteResult FxObjectSprite(uint16_t sprite_scale, uint16_t draw_list_end, uint16_t *stack)
{
    FxObjectSpriteResult r = object_sprite(sprite_scale, draw_list_end, stack);

    fx_solid_end();
    return r;
}

static FxObjectSpriteResult object_sprite(uint16_t sprite_scale, uint16_t draw_list_end, uint16_t *stack)
{
    int run_lB14C = 0;
    uint16_t place_x = 0, place_y = 0, place_size = 0;  /* the place projected, for fx_sprite_place */
    uint16_t x;
    uint16_t y;
    uint16_t ba;
    uint16_t bc;
    uint16_t c8;
    uint16_t ce;
    uint16_t x_2;
    uint16_t y_2;
    uint16_t ba_2;
    uint16_t bc_2;
    uint16_t c8_2;
    uint16_t ce_2;
    uint16_t ce_3;
    uint16_t ce_4;
    uint16_t count;
    uint16_t scale;
    uint16_t size_1;
    uint16_t count_2;
    uint16_t count_3;
    uint16_t count_4;
    uint16_t count_5;
    uint16_t count_6;
    uint16_t size_sum;
    uint16_t tex_bank;
    uint16_t tex_base;
    uint16_t tex_mask;
    uint16_t rows_left;
    uint16_t clip_width;
    uint16_t scale_word;
    uint16_t tex_mask_2;
    uint16_t texel_step;
    uint16_t clip_bottom;
    uint16_t pixel_count;
    uint16_t pulled_size;
    uint16_t sprite_size;
    uint16_t doubled_size;
    uint16_t next_start_x;
    uint16_t plot_start_x;
    uint16_t size_negated;
    uint32_t scale_product;
    uint16_t size_headroom;
    uint16_t sprite_clip_w;
    uint16_t sprite_clip_x;
    uint16_t sprite_colour;
    uint16_t sprite_left_x;
    uint16_t depth_positive;
    uint16_t mirror_start_x;
    uint16_t scale_low_byte;
    uint16_t sprite_visible;
    uint16_t width_overflow;
    uint16_t plot_x_mirrored;
    uint16_t sprite_clip_w_2;
    uint16_t sprite_material;
    uint16_t mirror_col_accum;
    uint16_t mirror_rows_left;
    uint16_t projection_scale;
    uint16_t scale_table_addr;
    uint16_t source_col_accum;
    uint16_t source_row_accum;
    int start_x_negative;
    uint16_t texture_end_addr;
    uint16_t screen_position_x;
    uint16_t screen_position_y;
    uint16_t sprite_size_field;
    uint16_t clip_width_recheck;
    uint16_t mirror_clip_bottom;
    uint16_t mirror_next_start_x;
    uint16_t screen_position_x_2;
    uint16_t mirror_width_overflow;
    int mirror_start_x_negative;
    uint16_t source_col_accum_clipped;

    fx_spill(stack);
    depth_positive = fx_ldw(0x002A);
    if (!((uint16_t)((uint16_t)(0 - depth_positive) + 0x0080) & 0x8000)) {
        fx_unspill(stack);
        return (FxObjectSpriteResult){draw_list_end, 0};
    } else {
        fx_solid_begin();
        ce = fx_ldw(FX_MODEL_CE);
        sprite_size_field = ce;
        if (ce & 0x8000) {
            fx_push(stack, sprite_size_field);
            x = fx_ldw(0x0026);
            y = fx_ldw(0x0028);
            { FxProjectScaledResult r_ = FxProjectScaled(x, y, depth_positive, sprite_scale); screen_position_x = r_.screen_position_x; screen_position_y = r_.screen_position_y; scale = r_.scale; };
            place_x = x;
            place_y = y;
            fx_stw(FX_MODEL_BA, screen_position_x);
            fx_stw(FX_MODEL_BC, screen_position_y);
            pulled_size = fx_pull(stack);
            size_negated = (uint16_t)((uint16_t)~pulled_size + 1);
            ce_2 = size_negated;
            if (size_negated != 0) {
                run_lB14C = 1;
            } else {
                fx_unspill(stack);
                return (FxObjectSpriteResult){draw_list_end, 0};
            }
        } else {
            count = fx_ldw(0x0032);
            count_2 = count;
            if (count == 0) {
            } else {
                doubled_size = sprite_size_field;
                for (;;) {
                    count_2 = (uint16_t)(count_2 - 1);
                    doubled_size = (uint16_t)(doubled_size + doubled_size);
                    if (count_2 != 0) {
                        continue;
                    } else {
                        break;
                    }
                }
                sprite_size_field = doubled_size;
            }
            sprite_size = fx_ldw(FX_SPRITE_SIZE);
            size_sum = (uint16_t)((uint16_t)(sprite_size + sprite_size) + sprite_size_field);
            projection_scale = size_sum;
            if (size_sum != 0) {
            } else {
                projection_scale = 1;
            }
            if (fx_far_shift()) {       /* a far sprite's size halved with its place (fx_far.c) */
                projection_scale = (uint16_t)(projection_scale >> fx_far_shift());
                if (!projection_scale)
                    projection_scale = 1;
            }
            x_2 = fx_ldw(0x0026);
            y_2 = fx_ldw(0x0028);
            { FxProjectScaledResult r_ = FxProjectScaled(x_2, y_2, depth_positive, projection_scale); screen_position_x_2 = r_.screen_position_x; screen_position_y = r_.screen_position_y; size_1 = r_.size_1; scale = r_.scale; };
            place_x = x_2;
            place_y = y_2;
            place_size = projection_scale;
            fx_stw(FX_MODEL_BA, screen_position_x_2);
            fx_stw(FX_MODEL_BC, screen_position_y);
            /* A zoomed view (fx_view.h) shows the sprite `zoom` times the
               size the chip works out, so one the chip rounds to nothing
               (a car's tyre far off: 50 * 128 / 17000, under half a pixel)
               is drawn while its zoomed size is half a pixel or more, from
               a box of one pixel, which fx_sprite_place maps onto its exact
               size; its solid (fx_solid.c) takes its own size as ever. */
            if (size_1 == 0 && fx_view_zoom() > 1.0f && depth_positive && !(depth_positive & 0x8000)
                && (double)projection_scale * 128.0 * fx_view_zoom() / depth_positive >= 0.5)
                size_1 = 1;
            ce_2 = size_1;
            if (size_1 != 0) {
                run_lB14C = 1;
            } else {
                fx_unspill(stack);
                return (FxObjectSpriteResult){draw_list_end, 0};
            }
        }
        size_headroom = (uint16_t)((uint16_t)(0 - ce_2) + 0x00F0);
        if (!(size_headroom & 0x8000)) {
        } else {
            ce_2 = (uint16_t)(size_headroom + ce_2);
        }
        fx_stw(FX_MODEL_CE, ce_2);
        fx_sprite_place(place_x, place_y, depth_positive, place_size, (uint16_t)(fx_ram[fx_ram_at(FX_MODEL_BA)] | fx_ram[fx_ram_at(FX_MODEL_BA ^ 1)] << 8),
                        (uint16_t)(fx_ram[fx_ram_at(FX_MODEL_BC)] | fx_ram[fx_ram_at(FX_MODEL_BC ^ 1)] << 8), ce_2);
        sprite_material = fx_ldw(FX_SPRITE_MATERIAL);
        FxSpriteMaterial(sprite_material, screen_position_y, scale);
        sprite_visible = fx_ldw(0x0042);
        if (sprite_visible == 0) {
            fx_unspill(stack);
            return (FxObjectSpriteResult){draw_list_end, 0};
        } else {
            ce_3 = fx_ldw(FX_MODEL_CE);
            tex_mask = fx_ldw(FX_TEX_MASK);
            fx_stw(FX_MODEL_C8, (uint16_t)((uint16_t)(tex_mask & 0xFF) + 1));
            sprite_colour = fx_ldw(0x0058);
            fx_color((uint8_t)sprite_colour);
            fx_cmode((uint16_t)(sprite_colour << 8 | sprite_colour >> 8));
            fx_solid_colour(fxs.colr, fxs.por);
            fx_stw(FX_SPRITE_CLIP_X, (uint16_t)((uint16_t)~ce_3 + 1));
            clip_width = fx_ldw(0x003A);
            fx_stw(FX_SPRITE_CLIP_W, (uint16_t)(clip_width - ce_3));
            ba = fx_ldw(FX_MODEL_BA);
            sprite_left_x = (uint16_t)(ba - (uint16_t)((uint16_t)(ce_3 >> 1) - 0xFFFF - !(ce_3 & 1)));
            fx_sbk(sprite_left_x);
            sprite_clip_x = fx_ldw(FX_SPRITE_CLIP_X);
            if (!((uint16_t)((uint16_t)(sprite_left_x - sprite_clip_x) - 1) & 0x8000)) {
                clip_width_recheck = fx_ldw(0x003A);
                if (!((uint16_t)(sprite_left_x - clip_width_recheck) & 0x8000)) {
                    fx_unspill(stack);
                    return (FxObjectSpriteResult){draw_list_end, 0};
                } else {
                    bc = fx_ldw(FX_MODEL_BC);
                    fx_sbk((uint16_t)(bc - (uint16_t)((uint16_t)(ce_3 >> 1) - 0xFFFF - !(ce_3 & 1))));
                    c8 = fx_ldw(FX_MODEL_C8);
                    if (ce_3 < c8) {
                        fx_romb(0);
                        scale_table_addr = (uint16_t)((uint16_t)(ce_3 + ce_3) - 0x6B60);
                        fx_r14(scale_table_addr);
                        scale_low_byte = fx_getb();
                        fx_r14((uint16_t)(scale_table_addr + 1));
                        scale_word = (uint16_t)((scale_low_byte & 0xFF) | fx_getb() << 8);
                        scale_product = (uint32_t)((int32_t)(int16_t)(uint16_t)(c8 << 8 | c8 >> 8) * (int16_t)scale_word);
                        texel_step = (uint16_t)((uint16_t)(scale_product >> 16) << 1 | (int)(scale_product >> 15 & 1));
                        bc_2 = fx_ldw(FX_MODEL_BC);
                        source_row_accum = 0;
                        source_col_accum_clipped = source_row_accum;
                        ce_4 = fx_ldw(FX_MODEL_CE);
                        pixel_count = ce_4;
                        tex_bank = fx_ldw(FX_TEX_BANK);
                        fx_romb(tex_bank);
                        tex_base = fx_ldw(FX_TEX_BASE);
                        ba_2 = fx_ldw(FX_MODEL_BA);
                        tex_mask_2 = fx_ldw(FX_TEX_MASK);
                        {
                            uint16_t accum = fx_sprite_shrink(ce_4, c8, texel_step, ba_2, bc_2, tex_bank, tex_base, tex_mask_2);

                            fx_unspill(stack);
                            return (FxObjectSpriteResult){accum, 0};
                        }
                    } else {
                        { FxObjectSpriteRowsResult tr_ = FxObjectSpriteRows(ce_3, c8, stack); return (FxObjectSpriteResult){tr_.row_error_out, tr_.up ? tr_.up == 1 ? 0 : tr_.up - 1 : 0}; }
                    }
                }
            } else {
                fx_unspill(stack);
                return (FxObjectSpriteResult){draw_list_end, 0};
            }
        }
    }
}


/*
 * @routine 01:B147 FxObjectSpritePull
 * WHAT IT DOES: FxObjectSprite's return when the size it pulled back at
 * 01:B13F negates to 0. The game never runs it: that size was pushed at
 * 01:B128 only when it is negative ($00CE), so its negation is never 0
 * and the BNE at 01:B144 always branches. A unit start runs it so the C
 * is judged against the original.
 * HOW: DEC R10 twice, then R15 = the word there (NOP in the delay slot).
 * Entry: R10 = stack, the return address the word below it. Exit: R10
 * two lower; S, Z from R10.
 */
FxObjectSpritePullResult FxObjectSpritePull(uint16_t *stack)
{

    fx_unspill(stack);
    return (FxObjectSpritePullResult){1};
}


/*
 * @routine 01:B17D FxObjectSpriteAt
 * WHAT IT DOES: A second way into FxObjectSprite's drawing (the word
 * $B17D is at 01:B0D2, in code no script has run): pushes R11 on the
 * R10 stack and goes on as FxObjectSprite does once its colour is set
 * (not reached in the traces so far).
 * HOW: FROM R11; STW (R10); INC R10 twice; then 01:B182.
 * Entry: R10 = stack; R11 = return address; as FxObjectSprite has them
 * at 01:B182.
 * Exit: as FxObjectSprite.
 */
FxObjectSpriteAtResult FxObjectSpriteAt(uint16_t sprite_width, uint16_t incoming_row, uint16_t *stack)
{
    uint16_t ba;
    uint16_t bc;
    uint16_t c8;
    uint16_t ce;
    uint16_t ba_2;
    uint16_t bc_2;
    uint16_t c8_2;
    uint16_t count;
    uint16_t count_2;
    uint16_t count_3;
    uint16_t count_4;
    uint16_t tex_bank;
    uint16_t tex_base;
    uint16_t tex_mask;
    uint16_t rows_left;
    uint16_t texel_step;
    uint16_t clip_bottom;
    uint16_t pixel_count;
    uint16_t scale_factor;
    uint16_t screen_width;
    uint32_t scale_product;
    uint16_t sprite_clip_w;
    uint16_t sprite_clip_x;
    uint16_t sprite_left_x;
    uint16_t scale_low_byte;
    uint16_t screen_width_2;
    uint16_t sprite_start_x;
    uint16_t clipped_start_x;
    uint16_t plot_x_mirrored;
    uint16_t right_overshoot;
    uint16_t scale_table_ptr;
    uint16_t sprite_clip_w_2;
    uint16_t source_col_accum;
    uint16_t source_row_accum;
    uint16_t mirrored_tex_base;
    uint16_t rows_left_mirrored;
    uint16_t clip_bottom_mirrored;
    int mirrored_starts_left;
    uint16_t source_col_accum_skip;
    int starts_left_of_screen;
    uint16_t sprite_start_x_mirrored;
    uint16_t clipped_start_x_mirrored;
    uint16_t right_overshoot_mirrored;
    uint16_t source_col_accum_mirrored;

    fx_spill(stack);
    fx_stw(FX_SPRITE_CLIP_X, (uint16_t)((uint16_t)~sprite_width + 1));
    screen_width = fx_ldw(0x003A);
    fx_stw(FX_SPRITE_CLIP_W, (uint16_t)(screen_width - sprite_width));
    ba = fx_ldw(FX_MODEL_BA);
    sprite_left_x = (uint16_t)(ba - (uint16_t)((uint16_t)(sprite_width >> 1) - 0xFFFF - !(sprite_width & 1)));
    fx_sbk(sprite_left_x);
    sprite_clip_x = fx_ldw(FX_SPRITE_CLIP_X);
    if (!((uint16_t)((uint16_t)(sprite_left_x - sprite_clip_x) - 1) & 0x8000)) {
        screen_width_2 = fx_ldw(0x003A);
        if (!((uint16_t)(sprite_left_x - screen_width_2) & 0x8000)) {
            fx_unspill(stack);
            return (FxObjectSpriteAtResult){sprite_width, incoming_row, 0};
        } else {
            bc = fx_ldw(FX_MODEL_BC);
            fx_sbk((uint16_t)(bc - (uint16_t)((uint16_t)(sprite_width >> 1) - 0xFFFF - !(sprite_width & 1))));
            c8 = fx_ldw(FX_MODEL_C8);
            if (sprite_width < c8) {
                fx_romb(0);
                scale_table_ptr = (uint16_t)((uint16_t)(sprite_width + sprite_width) - 0x6B60);
                fx_r14(scale_table_ptr);
                scale_low_byte = fx_getb();
                fx_r14((uint16_t)(scale_table_ptr + 1));
                scale_factor = (uint16_t)((scale_low_byte & 0xFF) | fx_getb() << 8);
                scale_product = (uint32_t)((int32_t)(int16_t)(uint16_t)(c8 << 8 | c8 >> 8) * (int16_t)scale_factor);
                texel_step = (uint16_t)((uint16_t)(scale_product >> 16) << 1 | (int)(scale_product >> 15 & 1));
                bc_2 = fx_ldw(FX_MODEL_BC);
                source_row_accum = 0;
                source_col_accum_skip = source_row_accum;
                ce = fx_ldw(FX_MODEL_CE);
                pixel_count = ce;
                tex_bank = fx_ldw(FX_TEX_BANK);
                fx_romb(tex_bank);
                tex_base = fx_ldw(FX_TEX_BASE);
                ba_2 = fx_ldw(FX_MODEL_BA);
                tex_mask = fx_ldw(FX_TEX_MASK);
                if (!(tex_mask & 0x8000)) {
                    starts_left_of_screen = ((ba_2 & 0x8000) != 0);
                    sprite_start_x = ba_2;
                    if (!starts_left_of_screen) {
                        sprite_clip_w = fx_ldw(FX_SPRITE_CLIP_W);
                        right_overshoot = (uint16_t)(sprite_clip_w - sprite_start_x);
                        if (!(right_overshoot & 0x8000)) {
                        } else {
                            pixel_count = (uint16_t)((uint16_t)(pixel_count + right_overshoot) + 1);
                        }
                        for (;;) {
                            clip_bottom = fx_ldw(0x003E);
                            if ((uint16_t)(clip_bottom - 1) < bc_2) {
                            } else {
                                source_col_accum = 0;
                                fx_r14((uint16_t)((uint16_t)((source_row_accum & 0xFF00) | source_col_accum >> 8) + tex_base));
                                sprite_width = sprite_start_x;
                                fx_row_open(1, sprite_width, bc_2, 0, texel_step, source_row_accum, tex_base, 0);
                                count = pixel_count;
                                for (;;) {
                                    source_col_accum = (uint16_t)(source_col_accum + texel_step);
                                    fx_color(fx_getb());
                                    fx_r14((uint16_t)((uint16_t)((source_row_accum & 0xFF00) | source_col_accum >> 8) + tex_base));
                                    count = (uint16_t)(count - 1);
                                    fx_plot((uint8_t)sprite_width, (uint8_t)bc_2);
                                    sprite_width = (uint16_t)(sprite_width + 1);
                                    if (count != 0) {
                                        continue;
                                    } else {
                                        break;
                                    }
                                }
                            }
                            fx_row_close(sprite_width);
                            source_row_accum = (uint16_t)(source_row_accum + texel_step);
                            rows_left = (uint16_t)(ce - 1);
                            ce = rows_left;
                            bc_2 = (uint16_t)(bc_2 + 1);
                            if (rows_left != 0) {
                                continue;
                            } else {
                                break;
                            }
                        }
                        fx_unspill(stack);
                        return (FxObjectSpriteAtResult){sprite_width, bc_2, 0};
                    } else {
                        count_2 = pixel_count;
                        for (;;) {
                            if (!starts_left_of_screen) {
                                break;
                            } else {
                                source_col_accum_skip = (uint16_t)(source_col_accum_skip + texel_step);
                                count_2 = (uint16_t)(count_2 - 1);
                                clipped_start_x = (uint16_t)(sprite_start_x + 1);
                                starts_left_of_screen = ((clipped_start_x & 0x8000) != 0);
                                sprite_start_x = clipped_start_x;
                                if (count_2 != 0) {
                                    continue;
                                } else {
                                    fx_unspill(stack);
                                    return (FxObjectSpriteAtResult){sprite_width, bc_2, 0};
                                }
                            }
                        }
                        pixel_count = count_2;
                        tex_base = (uint16_t)((uint16_t)((source_row_accum & 0xFF00) | source_col_accum_skip >> 8) + tex_base);
                        sprite_clip_w = fx_ldw(FX_SPRITE_CLIP_W);
                        right_overshoot = (uint16_t)(sprite_clip_w - sprite_start_x);
                        if (!(right_overshoot & 0x8000)) {
                        } else {
                            pixel_count = (uint16_t)((uint16_t)(pixel_count + right_overshoot) + 1);
                        }
                        for (;;) {
                            clip_bottom = fx_ldw(0x003E);
                            if ((uint16_t)(clip_bottom - 1) < bc_2) {
                            } else {
                                source_col_accum = 0;
                                fx_r14((uint16_t)((uint16_t)((source_row_accum & 0xFF00) | source_col_accum >> 8) + tex_base));
                                sprite_width = sprite_start_x;
                                fx_row_open(1, sprite_width, bc_2, 0, texel_step, source_row_accum, tex_base, 0);
                                count = pixel_count;
                                for (;;) {
                                    source_col_accum = (uint16_t)(source_col_accum + texel_step);
                                    fx_color(fx_getb());
                                    fx_r14((uint16_t)((uint16_t)((source_row_accum & 0xFF00) | source_col_accum >> 8) + tex_base));
                                    count = (uint16_t)(count - 1);
                                    fx_plot((uint8_t)sprite_width, (uint8_t)bc_2);
                                    sprite_width = (uint16_t)(sprite_width + 1);
                                    if (count != 0) {
                                        continue;
                                    } else {
                                        break;
                                    }
                                }
                            }
                            fx_row_close(sprite_width);
                            source_row_accum = (uint16_t)(source_row_accum + texel_step);
                            rows_left = (uint16_t)(ce - 1);
                            ce = rows_left;
                            bc_2 = (uint16_t)(bc_2 + 1);
                            if (rows_left != 0) {
                                continue;
                            } else {
                                break;
                            }
                        }
                        fx_unspill(stack);
                        return (FxObjectSpriteAtResult){sprite_width, bc_2, 0};
                    }
                } else {
                    c8_2 = fx_ldw(FX_MODEL_C8);
                    mirrored_tex_base = (uint16_t)((uint16_t)(tex_base + c8_2) - 1);
                    mirrored_starts_left = ((ba_2 & 0x8000) != 0);
                    sprite_start_x_mirrored = ba_2;
                    if (!mirrored_starts_left) {
                        plot_x_mirrored = sprite_start_x_mirrored;
                        sprite_clip_w_2 = fx_ldw(FX_SPRITE_CLIP_W);
                        right_overshoot_mirrored = (uint16_t)(sprite_clip_w_2 - plot_x_mirrored);
                        if (!(right_overshoot_mirrored & 0x8000)) {
                        } else {
                            pixel_count = (uint16_t)((uint16_t)(pixel_count + right_overshoot_mirrored) + 1);
                        }
                        for (;;) {
                            clip_bottom_mirrored = fx_ldw(0x003E);
                            if ((uint16_t)(clip_bottom_mirrored - 1) < bc_2) {
                            } else {
                                source_col_accum_mirrored = 0;
                                fx_r14((uint16_t)((uint16_t)((source_row_accum & 0xFF00) | source_col_accum_mirrored >> 8) + mirrored_tex_base));
                                plot_x_mirrored = sprite_start_x_mirrored;
                                fx_row_open(1, plot_x_mirrored, bc_2, 0, (uint16_t)-texel_step, source_row_accum, mirrored_tex_base, 0);
                                count_3 = pixel_count;
                                for (;;) {
                                    source_col_accum_mirrored = (uint16_t)(source_col_accum_mirrored - texel_step);
                                    fx_color(fx_getb());
                                    fx_r14((uint16_t)((uint16_t)((source_row_accum & 0xFF00) | source_col_accum_mirrored >> 8) + mirrored_tex_base));
                                    count_3 = (uint16_t)(count_3 - 1);
                                    fx_plot((uint8_t)plot_x_mirrored, (uint8_t)bc_2);
                                    plot_x_mirrored = (uint16_t)(plot_x_mirrored + 1);
                                    if (count_3 != 0) {
                                        continue;
                                    } else {
                                        break;
                                    }
                                }
                            }
                            fx_row_close(plot_x_mirrored);
                            source_row_accum = (uint16_t)(source_row_accum + texel_step);
                            rows_left_mirrored = (uint16_t)(ce - 1);
                            ce = rows_left_mirrored;
                            bc_2 = (uint16_t)(bc_2 + 1);
                            if (rows_left_mirrored != 0) {
                                continue;
                            } else {
                                break;
                            }
                        }
                        fx_unspill(stack);
                        return (FxObjectSpriteAtResult){plot_x_mirrored, bc_2, 0};
                    } else {
                        count_4 = pixel_count;
                        for (;;) {
                            if (!mirrored_starts_left) {
                                break;
                            } else {
                                source_col_accum_skip = (uint16_t)(source_col_accum_skip - texel_step);
                                count_4 = (uint16_t)(count_4 - 1);
                                clipped_start_x_mirrored = (uint16_t)(sprite_start_x_mirrored + 1);
                                mirrored_starts_left = ((clipped_start_x_mirrored & 0x8000) != 0);
                                sprite_start_x_mirrored = clipped_start_x_mirrored;
                                if (count_4 != 0) {
                                    continue;
                                } else {
                                    fx_unspill(stack);
                                    return (FxObjectSpriteAtResult){sprite_width, bc_2, 0};
                                }
                            }
                        }
                        pixel_count = count_4;
                        mirrored_tex_base = (uint16_t)((uint16_t)((source_row_accum & 0xFF00) | source_col_accum_skip >> 8) + mirrored_tex_base);
                        plot_x_mirrored = sprite_start_x_mirrored;
                        sprite_clip_w_2 = fx_ldw(FX_SPRITE_CLIP_W);
                        right_overshoot_mirrored = (uint16_t)(sprite_clip_w_2 - plot_x_mirrored);
                        if (!(right_overshoot_mirrored & 0x8000)) {
                        } else {
                            pixel_count = (uint16_t)((uint16_t)(pixel_count + right_overshoot_mirrored) + 1);
                        }
                        for (;;) {
                            clip_bottom_mirrored = fx_ldw(0x003E);
                            if ((uint16_t)(clip_bottom_mirrored - 1) < bc_2) {
                            } else {
                                source_col_accum_mirrored = 0;
                                fx_r14((uint16_t)((uint16_t)((source_row_accum & 0xFF00) | source_col_accum_mirrored >> 8) + mirrored_tex_base));
                                plot_x_mirrored = sprite_start_x_mirrored;
                                fx_row_open(1, plot_x_mirrored, bc_2, 0, (uint16_t)-texel_step, source_row_accum, mirrored_tex_base, 0);
                                count_3 = pixel_count;
                                for (;;) {
                                    source_col_accum_mirrored = (uint16_t)(source_col_accum_mirrored - texel_step);
                                    fx_color(fx_getb());
                                    fx_r14((uint16_t)((uint16_t)((source_row_accum & 0xFF00) | source_col_accum_mirrored >> 8) + mirrored_tex_base));
                                    count_3 = (uint16_t)(count_3 - 1);
                                    fx_plot((uint8_t)plot_x_mirrored, (uint8_t)bc_2);
                                    plot_x_mirrored = (uint16_t)(plot_x_mirrored + 1);
                                    if (count_3 != 0) {
                                        continue;
                                    } else {
                                        break;
                                    }
                                }
                            }
                            fx_row_close(plot_x_mirrored);
                            source_row_accum = (uint16_t)(source_row_accum + texel_step);
                            rows_left_mirrored = (uint16_t)(ce - 1);
                            ce = rows_left_mirrored;
                            bc_2 = (uint16_t)(bc_2 + 1);
                            if (rows_left_mirrored != 0) {
                                continue;
                            } else {
                                break;
                            }
                        }
                        fx_unspill(stack);
                        return (FxObjectSpriteAtResult){plot_x_mirrored, bc_2, 0};
                    }
                }
            } else {
                { FxObjectSpriteRowsResult tr_ = FxObjectSpriteRows(sprite_width, c8, stack); return (FxObjectSpriteAtResult){tr_.plot_end_x, tr_.plot_end_y, tr_.up ? tr_.up == 1 ? 0 : tr_.up - 1 : 0}; }
            }
        }
    } else {
        fx_unspill(stack);
        return (FxObjectSpriteAtResult){sprite_width, incoming_row, 0};
    }
}


/*
 * @routine 01:B2ED FxObjectSpriteRows
 * WHAT IT DOES: The rows of an object sprite: the texel step from the
 * size (FxDivide32By16Halving, or $FFFF when the size is the scale),
 * then each row of texels from the sprite's texture (bank $00A6,
 * address $00A8, mask $00D4) plotted between the screen's edges,
 * columns stepped by R9; mirrored when the mask's bit 15 is set
 * (01:B3AB).
 * HOW: Reached from FxObjectSprite (01:B1CA jumps here, after
 * FxProjectScaled's code ends at 01:B2EC). CACHE; GETC, PLOT in a LOOP
 * per row; returns by pulling R15 for FxObjectSprite.
 * Entry: as FxObjectSprite leaves them: R1 = the size, R6 = the scale;
 * R10 = stack.
 * Exit: as the code leaves the registers, flags and RAM; R14 and the
 * ROM buffer move as on the Super FX.
 */
FxObjectSpriteRowsResult FxObjectSpriteRows(uint16_t sprite_size, uint16_t scale, uint16_t *stack)
{
    uint16_t ba;
    uint16_t bc;
    uint16_t c8;
    uint16_t c8_2;
    uint16_t c8_3;
    uint16_t count;
    uint16_t left_x;
    uint16_t rom_at;
    uint16_t count_2;
    uint16_t divisor;
    uint16_t rom_at_2;
    uint16_t screen_y;
    uint16_t tex_bank;
    uint16_t tex_base;
    uint16_t tex_mask;
    uint16_t tex_rows;
    uint16_t row_error;
    uint16_t rows_left;
    uint32_t skip_prod;
    uint16_t tail_frac;
    uint16_t texel_row;
    uint16_t texel_step;
    uint16_t column_frac;
    int row_error_ok;
    uint16_t rows_to_draw;
    uint16_t rows_visible;
    uint16_t skip_prod_lo;
    int tail_wrapped;
    uint16_t left_x_mirror;
    uint16_t texels_in_row;
    int column_wrapped;
    uint16_t mirror_tex_end;
    uint16_t screen_right_x;
    uint16_t texels_skipped;
    uint32_t prod_step_width;
    uint16_t rows_after_skip;
    uint16_t screen_bottom_y;
    uint16_t screen_y_mirror;
    uint16_t tex_rows_mirror;
    uint16_t texel_step_even;
    uint16_t rows_left_mirror;
    uint32_t skip_prod_mirror;
    uint16_t tail_frac_mirror;
    uint16_t texel_row_mirror;
    uint16_t column_start_frac;
    uint16_t column_frac_mirror;
    int row_error_ok_mirror;
    uint16_t rows_to_draw_mirror;
    uint16_t rows_visible_mirror;
    uint16_t screen_right_x_tail;
    uint16_t skip_prod_lo_mirror;
    int tail_wrapped_mirror;
    uint16_t texels_in_row_mirror;
    int column_wrapped_mirror;
    uint16_t screen_right_x_mirror;
    uint16_t texels_skipped_mirror;
    uint32_t prod_step_width_mirror;
    uint16_t rows_after_skip_mirror;
    uint16_t screen_bottom_y_mirror;
    uint16_t screen_right_x_tail_mirror;

    divisor = sprite_size;
    if (scale != divisor) {
        texel_step = FxDivide32By16Halving(scale, 0, divisor);
    } else {
        texel_step = 0xFFFF;
    }
    c8 = fx_ldw(FX_MODEL_C8);
    ba = fx_ldw(FX_MODEL_BA);
    bc = fx_ldw(FX_MODEL_BC);
    row_error = 0;
    column_start_frac = 0;
    tex_bank = fx_ldw(FX_TEX_BANK);
    fx_romb(tex_bank);
    tex_base = fx_ldw(FX_TEX_BASE);
    texel_step_even = (uint16_t)(texel_step & ~1);
    tex_mask = fx_ldw(FX_TEX_MASK);
    {
        uint16_t error = fx_sprite_magnify(c8, texel_step_even, ba, bc, tex_bank, tex_base, tex_mask);

        fx_unspill(stack);
        return (FxObjectSpriteRowsResult){0, 0, error, 1};
    }
}


/*
 * @routine 01:B6D5 FxEffectsAdd
 * WHAT IT DOES: Adds R3 effect particles of kind R0 (1 to 7) at R1 / R2
 * to the twenty 14-byte slots at $273E (a slot with byte 0 free), each
 * kind's speeds, life and colour from the random seed at $01D0.
 * HOW: Pushes R11 on the R10 stack; ROMB $14, RAMB 0; a LOOP of R12 =
 * 20 (R13 = $B6F4) over the slots; the seed stepped as SWAP, ROR, ADD,
 * ADC; returns by pulling R15.
 * Entry: R0 = kind, R1, R2 = place, R3 = count; R10 = stack; R11 =
 * return address.
 * Exit: R0-R9 as the last slot left them; $01D0 = the seed; R12, R13
 * the loop; the rest as the code leaves them.
 */
void FxEffectsAdd(uint16_t kind, uint16_t place_x, uint16_t place, uint16_t count, uint16_t *stack)
{
    uint16_t count_2;
    uint16_t all_next;
    uint16_t all_prev;
    uint16_t slot_ptr;
    uint16_t random_seed;
    uint16_t kind6_speed_x;
    uint16_t kind6_speed_y;
    uint16_t kind6_seed_rot;
    uint16_t kind7_seed_rot;
    int kind_at_most_2;
    int kind_at_most_3;
    int kind_at_most_4;
    uint16_t particles_left;
    uint16_t kind2_seed_rot_1;
    uint16_t kind2_seed_rot_2;
    uint16_t kind2_seed_rot_3;
    uint16_t kind2_seed_rot_4;
    uint16_t kind2_seed_rot_5;
    uint16_t kind2_seed_sum_1;
    uint16_t kind3_seed_rot_1;
    uint16_t kind3_seed_rot_2;
    uint16_t kind3_seed_rot_3;
    uint16_t kind3_seed_rot_4;
    uint16_t kind3_seed_rot_5;
    uint16_t kind3_seed_sum_1;
    uint16_t kind4_seed_rot_1;
    uint16_t kind4_seed_rot_2;
    uint16_t kind4_seed_rot_3;
    uint16_t kind4_seed_rot_4;
    uint16_t kind4_seed_rot_5;
    uint16_t kind4_seed_sum_1;
    uint16_t kind6_frame_byte;
    uint16_t kind6_table_addr;
    uint16_t kind7_frame_byte;
    uint16_t kind7_table_addr;
    uint16_t slot_in_use_byte;
    uint16_t kind2_seed_next_1;
    uint16_t kind2_seed_next_2;
    uint16_t kind2_seed_next_3;
    uint16_t kind2_seed_next_4;
    uint16_t kind3_seed_next_1;
    uint16_t kind3_seed_next_2;
    uint16_t kind3_seed_next_3;
    uint16_t kind3_seed_next_4;
    uint16_t kind4_seed_next_1;
    uint16_t kind4_seed_next_2;
    uint16_t kind4_seed_next_3;
    uint16_t kind4_seed_next_4;
    uint16_t kind6_life_offset;
    uint16_t kind7_life_offset;
    uint16_t kind6_speed_x_half;
    uint16_t kind6_speed_y_half;
    uint16_t kind6_count_less_one;
    uint16_t kind7_count_less_one;
    uint16_t particles_left_after;

    fx_spill(stack);
    particles_left = count;
    fx_romb(0x0014);
    fx_ramb(0);
    slot_ptr = 0x273E;
    random_seed = fx_ldw(0x01D0);
    count_2 = 0x0014;
    for (;;) {
        slot_in_use_byte = fx_ldb(slot_ptr);
        if ((uint16_t)(slot_in_use_byte & 0xFF) != 0) {
            slot_ptr = (uint16_t)(slot_ptr + 0x000E);
            count_2 = (uint16_t)(count_2 - 1);
            if (count_2 != 0) {
                continue;
            } else {
                break;
            }
        } else {
            fx_stw((uint16_t)(slot_ptr + FXREC_ALL_PREV), 0);
            fx_stw((uint16_t)(slot_ptr + FXREC_POOL), 0);
            fx_stw((uint16_t)(slot_ptr + FX_PARTICLE_LIFE), 0);
            fx_stw((uint16_t)(slot_ptr + 0x000C), place);
            fx_stb(slot_ptr, (uint8_t)place_x);
            if ((uint16_t)((uint16_t)(0 - kind) + 1) != 0) {
                kind_at_most_2 = (2 >= kind);
                if ((uint16_t)((uint16_t)(0 - kind) + 2) != 0) {
                    kind_at_most_3 = (3 >= kind);
                    if ((uint16_t)((uint16_t)(0 - kind) + 3) != 0) {
                        kind_at_most_4 = (4 >= kind);
                        if ((uint16_t)((uint16_t)(0 - kind) + 4) != 0) {
                            if ((uint16_t)((uint16_t)(0 - kind) + 5) != 0) {
                                if ((uint16_t)((uint16_t)(0 - kind) + 6) != 0) {
                                    if ((uint16_t)((uint16_t)(0 - kind) + 7) != 0) {
                                    } else {
                                        fx_stb(slot_ptr, (uint8_t)(uint16_t)((uint16_t)(place_x + (particles_left & 0x000F)) - 7));
                                        fx_stb((uint16_t)(slot_ptr + 1), (uint8_t)0x0010);
                                        kind7_count_less_one = (uint16_t)(particles_left - 1);
                                        kind7_table_addr = (uint16_t)((uint16_t)(kind7_count_less_one + kind7_count_less_one) - 0x2F24);
                                        fx_r14(kind7_table_addr);
                                        all_prev = (uint16_t)(int16_t)(int8_t)fx_getb();
                                        fx_r14((uint16_t)(kind7_table_addr + 1));
                                        fx_stb((uint16_t)(slot_ptr + 3), (uint8_t)all_prev);
                                        fx_stw((uint16_t)(slot_ptr + FXREC_ALL_PREV), all_prev);
                                        all_next = (uint16_t)(int16_t)(int8_t)fx_getb();
                                        fx_stb((uint16_t)(slot_ptr + FXREC_ALL_NEXT), (uint8_t)all_next);
                                        fx_stw((uint16_t)(slot_ptr + FXREC_POOL), all_next);
                                        kind7_seed_rot = (uint16_t)((uint16_t)(random_seed << 8 | random_seed >> 8) >> 1 | ((uint32_t)8 + slot_ptr > 0xFFFF) << 15);
                                        random_seed = (uint16_t)((uint16_t)(kind7_seed_rot + random_seed) + random_seed + ((uint32_t)kind7_seed_rot + random_seed > 0xFFFF));
                                        kind7_life_offset = (uint16_t)((0x003F & random_seed) - 0x001F);
                                        fx_stb((uint16_t)(slot_ptr + 5), (uint8_t)kind7_life_offset);
                                        fx_stw((uint16_t)(slot_ptr + FX_PARTICLE_LIFE), kind7_life_offset);
                                        kind7_frame_byte = 4;
                                        if ((1 & count_2) == 0) {
                                        } else {
                                            kind7_frame_byte = 0x000E;
                                        }
                                        fx_stb((uint16_t)(slot_ptr + 2), (uint8_t)kind7_frame_byte);
                                    }
                                } else {
                                    fx_stb(slot_ptr, (uint8_t)(uint16_t)((uint16_t)(place_x + (particles_left & 0x000F)) - 7));
                                    fx_stb((uint16_t)(slot_ptr + 1), (uint8_t)0x0010);
                                    kind6_count_less_one = (uint16_t)(particles_left - 1);
                                    kind6_table_addr = (uint16_t)((uint16_t)(kind6_count_less_one + kind6_count_less_one) - 0x2F24);
                                    fx_r14(kind6_table_addr);
                                    kind6_speed_x = (uint16_t)(int16_t)(int8_t)fx_getb();
                                    fx_r14((uint16_t)(kind6_table_addr + 1));
                                    kind6_speed_x_half = fx_div2(kind6_speed_x);
                                    fx_stb((uint16_t)(slot_ptr + 3), (uint8_t)kind6_speed_x_half);
                                    fx_stw((uint16_t)(slot_ptr + FXREC_ALL_PREV), kind6_speed_x_half);
                                    kind6_speed_y = (uint16_t)(int16_t)(int8_t)fx_getb();
                                    kind6_speed_y_half = fx_div2(kind6_speed_y);
                                    fx_stb((uint16_t)(slot_ptr + FXREC_ALL_NEXT), (uint8_t)kind6_speed_y_half);
                                    fx_stw((uint16_t)(slot_ptr + FXREC_POOL), kind6_speed_y_half);
                                    kind6_seed_rot = (uint16_t)((uint16_t)(random_seed << 8 | random_seed >> 8) >> 1 | ((uint32_t)8 + slot_ptr > 0xFFFF) << 15);
                                    random_seed = (uint16_t)((uint16_t)(kind6_seed_rot + random_seed) + random_seed + ((uint32_t)kind6_seed_rot + random_seed > 0xFFFF));
                                    kind6_life_offset = (uint16_t)((0x001F & random_seed) - 0x000F);
                                    fx_stb((uint16_t)(slot_ptr + 5), (uint8_t)kind6_life_offset);
                                    fx_stw((uint16_t)(slot_ptr + FX_PARTICLE_LIFE), kind6_life_offset);
                                    kind6_frame_byte = 4;
                                    if ((1 & count_2) == 0) {
                                    } else {
                                        kind6_frame_byte = 0x000E;
                                    }
                                    fx_stb((uint16_t)(slot_ptr + 2), (uint8_t)kind6_frame_byte);
                                }
                            } else {
                            }
                        } else {
                            kind4_seed_rot_1 = (uint16_t)((uint16_t)(random_seed << 8 | random_seed >> 8) >> 1 | kind_at_most_4 << 15);
                            kind4_seed_sum_1 = (uint16_t)(kind4_seed_rot_1 + random_seed);
                            kind4_seed_next_1 = (uint16_t)(kind4_seed_sum_1 + random_seed + ((uint32_t)kind4_seed_rot_1 + random_seed > 0xFFFF));
                            fx_stb(slot_ptr, (uint8_t)(uint16_t)(place_x - (kind4_seed_sum_1 & 7)));
                            fx_stb((uint16_t)(slot_ptr + 1), (uint8_t)4);
                            kind4_seed_rot_2 = (uint16_t)((uint16_t)(kind4_seed_next_1 << 8 | kind4_seed_next_1 >> 8) >> 1 | ((uint32_t)1 + slot_ptr > 0xFFFF) << 15);
                            kind4_seed_next_2 = (uint16_t)((uint16_t)(kind4_seed_rot_2 + kind4_seed_next_1) + kind4_seed_next_1 + ((uint32_t)kind4_seed_rot_2 + kind4_seed_next_1 > 0xFFFF));
                            fx_stb((uint16_t)(slot_ptr + 3), (uint8_t)(uint16_t)((0x000F & kind4_seed_next_2) - 7));
                            kind4_seed_rot_3 = (uint16_t)((uint16_t)(kind4_seed_next_2 << 8 | kind4_seed_next_2 >> 8) >> 1 | ((uint32_t)3 + slot_ptr > 0xFFFF) << 15);
                            kind4_seed_next_3 = (uint16_t)((uint16_t)(kind4_seed_rot_3 + kind4_seed_next_2) + kind4_seed_next_2 + ((uint32_t)kind4_seed_rot_3 + kind4_seed_next_2 > 0xFFFF));
                            fx_stb((uint16_t)(slot_ptr + FXREC_ALL_NEXT), (uint8_t)(uint16_t)((7 & kind4_seed_next_3) - 3));
                            kind4_seed_rot_4 = (uint16_t)((uint16_t)(kind4_seed_next_3 << 8 | kind4_seed_next_3 >> 8) >> 1 | ((uint32_t)4 + slot_ptr > 0xFFFF) << 15);
                            kind4_seed_next_4 = (uint16_t)((uint16_t)(kind4_seed_rot_4 + kind4_seed_next_3) + kind4_seed_next_3 + ((uint32_t)kind4_seed_rot_4 + kind4_seed_next_3 > 0xFFFF));
                            fx_stb((uint16_t)(slot_ptr + 5), (uint8_t)(uint16_t)((uint16_t)~(0x001F & kind4_seed_next_4) + 1));
                            kind4_seed_rot_5 = (uint16_t)((uint16_t)(kind4_seed_next_4 << 8 | kind4_seed_next_4 >> 8) >> 1 | ((uint32_t)5 + slot_ptr > 0xFFFF) << 15);
                            random_seed = (uint16_t)((uint16_t)(kind4_seed_rot_5 + kind4_seed_next_4) + kind4_seed_next_4 + ((uint32_t)kind4_seed_rot_5 + kind4_seed_next_4 > 0xFFFF));
                            fx_stb((uint16_t)(slot_ptr + 2), (uint8_t)(uint16_t)((3 & random_seed) + 0x0068));
                        }
                    } else {
                        kind3_seed_rot_1 = (uint16_t)((uint16_t)(random_seed << 8 | random_seed >> 8) >> 1 | kind_at_most_3 << 15);
                        kind3_seed_sum_1 = (uint16_t)(kind3_seed_rot_1 + random_seed);
                        kind3_seed_next_1 = (uint16_t)(kind3_seed_sum_1 + random_seed + ((uint32_t)kind3_seed_rot_1 + random_seed > 0xFFFF));
                        fx_stb(slot_ptr, (uint8_t)(uint16_t)(place_x - (kind3_seed_sum_1 & 7)));
                        fx_stb((uint16_t)(slot_ptr + 1), (uint8_t)0);
                        kind3_seed_rot_2 = (uint16_t)((uint16_t)(kind3_seed_next_1 << 8 | kind3_seed_next_1 >> 8) >> 1 | ((uint32_t)1 + slot_ptr > 0xFFFF) << 15);
                        kind3_seed_next_2 = (uint16_t)((uint16_t)(kind3_seed_rot_2 + kind3_seed_next_1) + kind3_seed_next_1 + ((uint32_t)kind3_seed_rot_2 + kind3_seed_next_1 > 0xFFFF));
                        fx_stb((uint16_t)(slot_ptr + 3), (uint8_t)(uint16_t)((0x000F & kind3_seed_next_2) - 7));
                        kind3_seed_rot_3 = (uint16_t)((uint16_t)(kind3_seed_next_2 << 8 | kind3_seed_next_2 >> 8) >> 1 | ((uint32_t)3 + slot_ptr > 0xFFFF) << 15);
                        kind3_seed_next_3 = (uint16_t)((uint16_t)(kind3_seed_rot_3 + kind3_seed_next_2) + kind3_seed_next_2 + ((uint32_t)kind3_seed_rot_3 + kind3_seed_next_2 > 0xFFFF));
                        fx_stb((uint16_t)(slot_ptr + FXREC_ALL_NEXT), (uint8_t)(uint16_t)((0x000F & kind3_seed_next_3) - 7));
                        kind3_seed_rot_4 = (uint16_t)((uint16_t)(kind3_seed_next_3 << 8 | kind3_seed_next_3 >> 8) >> 1 | ((uint32_t)4 + slot_ptr > 0xFFFF) << 15);
                        kind3_seed_next_4 = (uint16_t)((uint16_t)(kind3_seed_rot_4 + kind3_seed_next_3) + kind3_seed_next_3 + ((uint32_t)kind3_seed_rot_4 + kind3_seed_next_3 > 0xFFFF));
                        fx_stb((uint16_t)(slot_ptr + 5), (uint8_t)(uint16_t)((uint16_t)~(0x007F & kind3_seed_next_4) + 1));
                        kind3_seed_rot_5 = (uint16_t)((uint16_t)(kind3_seed_next_4 << 8 | kind3_seed_next_4 >> 8) >> 1 | ((uint32_t)5 + slot_ptr > 0xFFFF) << 15);
                        random_seed = (uint16_t)((uint16_t)(kind3_seed_rot_5 + kind3_seed_next_4) + kind3_seed_next_4 + ((uint32_t)kind3_seed_rot_5 + kind3_seed_next_4 > 0xFFFF));
                        fx_stb((uint16_t)(slot_ptr + 2), (uint8_t)(uint16_t)((3 & random_seed) + 0x0083));
                    }
                } else {
                    kind2_seed_rot_1 = (uint16_t)((uint16_t)(random_seed << 8 | random_seed >> 8) >> 1 | kind_at_most_2 << 15);
                    kind2_seed_sum_1 = (uint16_t)(kind2_seed_rot_1 + random_seed);
                    kind2_seed_next_1 = (uint16_t)(kind2_seed_sum_1 + random_seed + ((uint32_t)kind2_seed_rot_1 + random_seed > 0xFFFF));
                    fx_stb(slot_ptr, (uint8_t)(uint16_t)(place_x - (kind2_seed_sum_1 & 7)));
                    fx_stb((uint16_t)(slot_ptr + 1), (uint8_t)2);
                    kind2_seed_rot_2 = (uint16_t)((uint16_t)(kind2_seed_next_1 << 8 | kind2_seed_next_1 >> 8) >> 1 | ((uint32_t)1 + slot_ptr > 0xFFFF) << 15);
                    kind2_seed_next_2 = (uint16_t)((uint16_t)(kind2_seed_rot_2 + kind2_seed_next_1) + kind2_seed_next_1 + ((uint32_t)kind2_seed_rot_2 + kind2_seed_next_1 > 0xFFFF));
                    fx_stb((uint16_t)(slot_ptr + 3), (uint8_t)(uint16_t)((0x000F & kind2_seed_next_2) - 7));
                    kind2_seed_rot_3 = (uint16_t)((uint16_t)(kind2_seed_next_2 << 8 | kind2_seed_next_2 >> 8) >> 1 | ((uint32_t)3 + slot_ptr > 0xFFFF) << 15);
                    kind2_seed_next_3 = (uint16_t)((uint16_t)(kind2_seed_rot_3 + kind2_seed_next_2) + kind2_seed_next_2 + ((uint32_t)kind2_seed_rot_3 + kind2_seed_next_2 > 0xFFFF));
                    fx_stb((uint16_t)(slot_ptr + FXREC_ALL_NEXT), (uint8_t)(uint16_t)((7 & kind2_seed_next_3) - 3));
                    kind2_seed_rot_4 = (uint16_t)((uint16_t)(kind2_seed_next_3 << 8 | kind2_seed_next_3 >> 8) >> 1 | ((uint32_t)4 + slot_ptr > 0xFFFF) << 15);
                    kind2_seed_next_4 = (uint16_t)((uint16_t)(kind2_seed_rot_4 + kind2_seed_next_3) + kind2_seed_next_3 + ((uint32_t)kind2_seed_rot_4 + kind2_seed_next_3 > 0xFFFF));
                    fx_stb((uint16_t)(slot_ptr + 5), (uint8_t)(uint16_t)((uint16_t)~(0x001F & kind2_seed_next_4) + 1));
                    kind2_seed_rot_5 = (uint16_t)((uint16_t)(kind2_seed_next_4 << 8 | kind2_seed_next_4 >> 8) >> 1 | ((uint32_t)5 + slot_ptr > 0xFFFF) << 15);
                    random_seed = (uint16_t)((uint16_t)(kind2_seed_rot_5 + kind2_seed_next_4) + kind2_seed_next_4 + ((uint32_t)kind2_seed_rot_5 + kind2_seed_next_4 > 0xFFFF));
                    fx_stb((uint16_t)(slot_ptr + 2), (uint8_t)(uint16_t)((3 & random_seed) + 0x0068));
                }
            } else {
            }
            particles_left_after = (uint16_t)(particles_left - 1);
            particles_left = particles_left_after;
            if (particles_left_after == 0) {
                break;
            } else {
                slot_ptr = (uint16_t)(slot_ptr + 0x000E);
                count_2 = (uint16_t)(count_2 - 1);
                if (count_2 != 0) {
                    continue;
                } else {
                    break;
                }
            }
        }
    }
    fx_stw(0x01D0, random_seed);
    fx_unspill(stack);
    return;
}


/*
 * @routine 01:B763 FxEffectsAddNext
 * WHAT IT DOES: FxEffectsAdd's step to the next particle: one fewer to
 * add (R9); when none are left, or after the last of the twenty slots,
 * the seed is kept at $01D0 and FxEffectsAdd returns.
 * HOW: DEC R9; the next slot R3 + 14 by LOOP (R13 = $B6F4); the return
 * pulled from the R10 stack. Every kind's set-up jumps here.
 * Entry: as FxEffectsAdd leaves them in its slot loop.
 * Exit: as FxEffectsAdd.
 */
FxEffectsAddNextResult FxEffectsAddNext(uint16_t effect_seed, uint16_t particle_slot_ptr, uint16_t place_y, uint16_t place_x, uint16_t effect_kind, uint16_t particles_left, uint16_t *stack, uint16_t count)
{
    int escape = 0;                    /* a way out of loops within loops */
    uint16_t all_next;
    uint16_t all_prev;
    uint16_t seed_rot_1;
    uint16_t seed_rot_2;
    uint16_t seed_rot_3;
    uint16_t seed_rot_4;
    uint16_t seed_rot_5;
    uint16_t seed_sum_1;
    uint16_t seed_next_1;
    uint16_t seed_next_2;
    uint16_t seed_next_3;
    uint16_t seed_next_4;
    uint16_t kind6_speed_x;
    uint16_t kind6_speed_y;
    uint16_t kind6_seed_rot;
    uint16_t kind7_seed_rot;
    int kind_at_most_2;
    int kind_at_most_3;
    int kind_at_most_4;
    uint16_t kind2_seed_rot_1;
    uint16_t kind2_seed_rot_2;
    uint16_t kind2_seed_rot_3;
    uint16_t kind2_seed_rot_4;
    uint16_t kind2_seed_rot_5;
    uint16_t kind2_seed_sum_1;
    uint16_t kind4_seed_rot_1;
    uint16_t kind4_seed_rot_2;
    uint16_t kind4_seed_rot_3;
    uint16_t kind4_seed_rot_4;
    uint16_t kind4_seed_rot_5;
    uint16_t kind4_seed_sum_1;
    uint16_t kind6_frame_byte;
    uint16_t kind6_table_addr;
    uint16_t kind7_frame_byte;
    uint16_t kind7_table_addr;
    uint16_t slot_in_use_byte;
    uint16_t kind2_seed_next_1;
    uint16_t kind2_seed_next_2;
    uint16_t kind2_seed_next_3;
    uint16_t kind2_seed_next_4;
    uint16_t kind4_seed_next_1;
    uint16_t kind4_seed_next_2;
    uint16_t kind4_seed_next_3;
    uint16_t kind4_seed_next_4;
    uint16_t kind6_life_offset;
    uint16_t kind7_life_offset;
    uint16_t kind6_speed_x_half;
    uint16_t kind6_speed_y_half;
    uint16_t kind6_count_less_one;
    uint16_t kind7_count_less_one;
    uint16_t particles_left_after;

    for (;;) {
        particles_left_after = (uint16_t)(particles_left - 1);
        particles_left = particles_left_after;
        if (particles_left_after == 0) {
            break;
        } else {
            for (;;) {
                particle_slot_ptr = (uint16_t)(particle_slot_ptr + 0x000E);
                count = (uint16_t)(count - 1);
                if (count != 0) {
                    slot_in_use_byte = fx_ldb(particle_slot_ptr);
                    if ((uint16_t)(slot_in_use_byte & 0xFF) != 0) {
                        continue;
                    } else {
                        fx_stw((uint16_t)(particle_slot_ptr + FXREC_ALL_PREV), 0);
                        fx_stw((uint16_t)(particle_slot_ptr + FXREC_POOL), 0);
                        fx_stw((uint16_t)(particle_slot_ptr + FX_PARTICLE_LIFE), 0);
                        fx_stw((uint16_t)(particle_slot_ptr + 0x000C), place_y);
                        fx_stb(particle_slot_ptr, (uint8_t)place_x);
                        if ((uint16_t)((uint16_t)(0 - effect_kind) + 1) != 0) {
                            kind_at_most_2 = (2 >= effect_kind);
                            if ((uint16_t)((uint16_t)(0 - effect_kind) + 2) != 0) {
                                kind_at_most_3 = (3 >= effect_kind);
                                if ((uint16_t)((uint16_t)(0 - effect_kind) + 3) != 0) {
                                    kind_at_most_4 = (4 >= effect_kind);
                                    if ((uint16_t)((uint16_t)(0 - effect_kind) + 4) != 0) {
                                        if ((uint16_t)((uint16_t)(0 - effect_kind) + 5) != 0) {
                                            if ((uint16_t)((uint16_t)(0 - effect_kind) + 6) != 0) {
                                                if ((uint16_t)((uint16_t)(0 - effect_kind) + 7) != 0) {
                                                    escape = 1;          /* continue lB763 */
                                                    break;
                                                } else {
                                                    fx_stb(particle_slot_ptr, (uint8_t)(uint16_t)((uint16_t)(place_x + (particles_left & 0x000F)) - 7));
                                                    fx_stb((uint16_t)(particle_slot_ptr + 1), (uint8_t)0x0010);
                                                    kind7_count_less_one = (uint16_t)(particles_left - 1);
                                                    kind7_table_addr = (uint16_t)((uint16_t)(kind7_count_less_one + kind7_count_less_one) - 0x2F24);
                                                    fx_r14(kind7_table_addr);
                                                    all_prev = (uint16_t)(int16_t)(int8_t)fx_getb();
                                                    fx_r14((uint16_t)(kind7_table_addr + 1));
                                                    fx_stb((uint16_t)(particle_slot_ptr + 3), (uint8_t)all_prev);
                                                    fx_stw((uint16_t)(particle_slot_ptr + FXREC_ALL_PREV), all_prev);
                                                    all_next = (uint16_t)(int16_t)(int8_t)fx_getb();
                                                    fx_stb((uint16_t)(particle_slot_ptr + FXREC_ALL_NEXT), (uint8_t)all_next);
                                                    fx_stw((uint16_t)(particle_slot_ptr + FXREC_POOL), all_next);
                                                    kind7_seed_rot = (uint16_t)((uint16_t)(effect_seed << 8 | effect_seed >> 8) >> 1 | ((uint32_t)8 + particle_slot_ptr > 0xFFFF) << 15);
                                                    effect_seed = (uint16_t)((uint16_t)(kind7_seed_rot + effect_seed) + effect_seed + ((uint32_t)kind7_seed_rot + effect_seed > 0xFFFF));
                                                    kind7_life_offset = (uint16_t)((0x003F & effect_seed) - 0x001F);
                                                    fx_stb((uint16_t)(particle_slot_ptr + 5), (uint8_t)kind7_life_offset);
                                                    fx_stw((uint16_t)(particle_slot_ptr + FX_PARTICLE_LIFE), kind7_life_offset);
                                                    kind7_frame_byte = 4;
                                                    if ((1 & count) == 0) {
                                                    } else {
                                                        kind7_frame_byte = 0x000E;
                                                    }
                                                    fx_stb((uint16_t)(particle_slot_ptr + 2), (uint8_t)kind7_frame_byte);
                                                    escape = 1;          /* continue lB763 */
                                                    break;
                                                }
                                            } else {
                                                fx_stb(particle_slot_ptr, (uint8_t)(uint16_t)((uint16_t)(place_x + (particles_left & 0x000F)) - 7));
                                                fx_stb((uint16_t)(particle_slot_ptr + 1), (uint8_t)0x0010);
                                                kind6_count_less_one = (uint16_t)(particles_left - 1);
                                                kind6_table_addr = (uint16_t)((uint16_t)(kind6_count_less_one + kind6_count_less_one) - 0x2F24);
                                                fx_r14(kind6_table_addr);
                                                kind6_speed_x = (uint16_t)(int16_t)(int8_t)fx_getb();
                                                fx_r14((uint16_t)(kind6_table_addr + 1));
                                                kind6_speed_x_half = fx_div2(kind6_speed_x);
                                                fx_stb((uint16_t)(particle_slot_ptr + 3), (uint8_t)kind6_speed_x_half);
                                                fx_stw((uint16_t)(particle_slot_ptr + FXREC_ALL_PREV), kind6_speed_x_half);
                                                kind6_speed_y = (uint16_t)(int16_t)(int8_t)fx_getb();
                                                kind6_speed_y_half = fx_div2(kind6_speed_y);
                                                fx_stb((uint16_t)(particle_slot_ptr + FXREC_ALL_NEXT), (uint8_t)kind6_speed_y_half);
                                                fx_stw((uint16_t)(particle_slot_ptr + FXREC_POOL), kind6_speed_y_half);
                                                kind6_seed_rot = (uint16_t)((uint16_t)(effect_seed << 8 | effect_seed >> 8) >> 1 | ((uint32_t)8 + particle_slot_ptr > 0xFFFF) << 15);
                                                effect_seed = (uint16_t)((uint16_t)(kind6_seed_rot + effect_seed) + effect_seed + ((uint32_t)kind6_seed_rot + effect_seed > 0xFFFF));
                                                kind6_life_offset = (uint16_t)((0x001F & effect_seed) - 0x000F);
                                                fx_stb((uint16_t)(particle_slot_ptr + 5), (uint8_t)kind6_life_offset);
                                                fx_stw((uint16_t)(particle_slot_ptr + FX_PARTICLE_LIFE), kind6_life_offset);
                                                kind6_frame_byte = 4;
                                                if ((1 & count) == 0) {
                                                } else {
                                                    kind6_frame_byte = 0x000E;
                                                }
                                                fx_stb((uint16_t)(particle_slot_ptr + 2), (uint8_t)kind6_frame_byte);
                                                escape = 1;          /* continue lB763 */
                                                break;
                                            }
                                        } else {
                                            escape = 1;          /* continue lB763 */
                                            break;
                                        }
                                    } else {
                                        kind4_seed_rot_1 = (uint16_t)((uint16_t)(effect_seed << 8 | effect_seed >> 8) >> 1 | kind_at_most_4 << 15);
                                        kind4_seed_sum_1 = (uint16_t)(kind4_seed_rot_1 + effect_seed);
                                        kind4_seed_next_1 = (uint16_t)(kind4_seed_sum_1 + effect_seed + ((uint32_t)kind4_seed_rot_1 + effect_seed > 0xFFFF));
                                        fx_stb(particle_slot_ptr, (uint8_t)(uint16_t)(place_x - (kind4_seed_sum_1 & 7)));
                                        fx_stb((uint16_t)(particle_slot_ptr + 1), (uint8_t)4);
                                        kind4_seed_rot_2 = (uint16_t)((uint16_t)(kind4_seed_next_1 << 8 | kind4_seed_next_1 >> 8) >> 1 | ((uint32_t)1 + particle_slot_ptr > 0xFFFF) << 15);
                                        kind4_seed_next_2 = (uint16_t)((uint16_t)(kind4_seed_rot_2 + kind4_seed_next_1) + kind4_seed_next_1 + ((uint32_t)kind4_seed_rot_2 + kind4_seed_next_1 > 0xFFFF));
                                        fx_stb((uint16_t)(particle_slot_ptr + 3), (uint8_t)(uint16_t)((0x000F & kind4_seed_next_2) - 7));
                                        kind4_seed_rot_3 = (uint16_t)((uint16_t)(kind4_seed_next_2 << 8 | kind4_seed_next_2 >> 8) >> 1 | ((uint32_t)3 + particle_slot_ptr > 0xFFFF) << 15);
                                        kind4_seed_next_3 = (uint16_t)((uint16_t)(kind4_seed_rot_3 + kind4_seed_next_2) + kind4_seed_next_2 + ((uint32_t)kind4_seed_rot_3 + kind4_seed_next_2 > 0xFFFF));
                                        fx_stb((uint16_t)(particle_slot_ptr + FXREC_ALL_NEXT), (uint8_t)(uint16_t)((7 & kind4_seed_next_3) - 3));
                                        kind4_seed_rot_4 = (uint16_t)((uint16_t)(kind4_seed_next_3 << 8 | kind4_seed_next_3 >> 8) >> 1 | ((uint32_t)4 + particle_slot_ptr > 0xFFFF) << 15);
                                        kind4_seed_next_4 = (uint16_t)((uint16_t)(kind4_seed_rot_4 + kind4_seed_next_3) + kind4_seed_next_3 + ((uint32_t)kind4_seed_rot_4 + kind4_seed_next_3 > 0xFFFF));
                                        fx_stb((uint16_t)(particle_slot_ptr + 5), (uint8_t)(uint16_t)((uint16_t)~(0x001F & kind4_seed_next_4) + 1));
                                        kind4_seed_rot_5 = (uint16_t)((uint16_t)(kind4_seed_next_4 << 8 | kind4_seed_next_4 >> 8) >> 1 | ((uint32_t)5 + particle_slot_ptr > 0xFFFF) << 15);
                                        effect_seed = (uint16_t)((uint16_t)(kind4_seed_rot_5 + kind4_seed_next_4) + kind4_seed_next_4 + ((uint32_t)kind4_seed_rot_5 + kind4_seed_next_4 > 0xFFFF));
                                        fx_stb((uint16_t)(particle_slot_ptr + 2), (uint8_t)(uint16_t)((3 & effect_seed) + 0x0068));
                                        escape = 1;          /* continue lB763 */
                                        break;
                                    }
                                } else {
                                    seed_rot_1 = (uint16_t)((uint16_t)(effect_seed << 8 | effect_seed >> 8) >> 1 | kind_at_most_3 << 15);
                                    seed_sum_1 = (uint16_t)(seed_rot_1 + effect_seed);
                                    seed_next_1 = (uint16_t)(seed_sum_1 + effect_seed + ((uint32_t)seed_rot_1 + effect_seed > 0xFFFF));
                                    fx_stb(particle_slot_ptr, (uint8_t)(uint16_t)(place_x - (seed_sum_1 & 7)));
                                    fx_stb((uint16_t)(particle_slot_ptr + 1), (uint8_t)0);
                                    seed_rot_2 = (uint16_t)((uint16_t)(seed_next_1 << 8 | seed_next_1 >> 8) >> 1 | ((uint32_t)1 + particle_slot_ptr > 0xFFFF) << 15);
                                    seed_next_2 = (uint16_t)((uint16_t)(seed_rot_2 + seed_next_1) + seed_next_1 + ((uint32_t)seed_rot_2 + seed_next_1 > 0xFFFF));
                                    fx_stb((uint16_t)(particle_slot_ptr + 3), (uint8_t)(uint16_t)((0x000F & seed_next_2) - 7));
                                    seed_rot_3 = (uint16_t)((uint16_t)(seed_next_2 << 8 | seed_next_2 >> 8) >> 1 | ((uint32_t)3 + particle_slot_ptr > 0xFFFF) << 15);
                                    seed_next_3 = (uint16_t)((uint16_t)(seed_rot_3 + seed_next_2) + seed_next_2 + ((uint32_t)seed_rot_3 + seed_next_2 > 0xFFFF));
                                    fx_stb((uint16_t)(particle_slot_ptr + FXREC_ALL_NEXT), (uint8_t)(uint16_t)((0x000F & seed_next_3) - 7));
                                    seed_rot_4 = (uint16_t)((uint16_t)(seed_next_3 << 8 | seed_next_3 >> 8) >> 1 | ((uint32_t)4 + particle_slot_ptr > 0xFFFF) << 15);
                                    seed_next_4 = (uint16_t)((uint16_t)(seed_rot_4 + seed_next_3) + seed_next_3 + ((uint32_t)seed_rot_4 + seed_next_3 > 0xFFFF));
                                    fx_stb((uint16_t)(particle_slot_ptr + 5), (uint8_t)(uint16_t)((uint16_t)~(0x007F & seed_next_4) + 1));
                                    seed_rot_5 = (uint16_t)((uint16_t)(seed_next_4 << 8 | seed_next_4 >> 8) >> 1 | ((uint32_t)5 + particle_slot_ptr > 0xFFFF) << 15);
                                    effect_seed = (uint16_t)((uint16_t)(seed_rot_5 + seed_next_4) + seed_next_4 + ((uint32_t)seed_rot_5 + seed_next_4 > 0xFFFF));
                                    fx_stb((uint16_t)(particle_slot_ptr + 2), (uint8_t)(uint16_t)((3 & effect_seed) + 0x0083));
                                    escape = 1;          /* continue lB763 */
                                    break;
                                }
                            } else {
                                kind2_seed_rot_1 = (uint16_t)((uint16_t)(effect_seed << 8 | effect_seed >> 8) >> 1 | kind_at_most_2 << 15);
                                kind2_seed_sum_1 = (uint16_t)(kind2_seed_rot_1 + effect_seed);
                                kind2_seed_next_1 = (uint16_t)(kind2_seed_sum_1 + effect_seed + ((uint32_t)kind2_seed_rot_1 + effect_seed > 0xFFFF));
                                fx_stb(particle_slot_ptr, (uint8_t)(uint16_t)(place_x - (kind2_seed_sum_1 & 7)));
                                fx_stb((uint16_t)(particle_slot_ptr + 1), (uint8_t)2);
                                kind2_seed_rot_2 = (uint16_t)((uint16_t)(kind2_seed_next_1 << 8 | kind2_seed_next_1 >> 8) >> 1 | ((uint32_t)1 + particle_slot_ptr > 0xFFFF) << 15);
                                kind2_seed_next_2 = (uint16_t)((uint16_t)(kind2_seed_rot_2 + kind2_seed_next_1) + kind2_seed_next_1 + ((uint32_t)kind2_seed_rot_2 + kind2_seed_next_1 > 0xFFFF));
                                fx_stb((uint16_t)(particle_slot_ptr + 3), (uint8_t)(uint16_t)((0x000F & kind2_seed_next_2) - 7));
                                kind2_seed_rot_3 = (uint16_t)((uint16_t)(kind2_seed_next_2 << 8 | kind2_seed_next_2 >> 8) >> 1 | ((uint32_t)3 + particle_slot_ptr > 0xFFFF) << 15);
                                kind2_seed_next_3 = (uint16_t)((uint16_t)(kind2_seed_rot_3 + kind2_seed_next_2) + kind2_seed_next_2 + ((uint32_t)kind2_seed_rot_3 + kind2_seed_next_2 > 0xFFFF));
                                fx_stb((uint16_t)(particle_slot_ptr + FXREC_ALL_NEXT), (uint8_t)(uint16_t)((7 & kind2_seed_next_3) - 3));
                                kind2_seed_rot_4 = (uint16_t)((uint16_t)(kind2_seed_next_3 << 8 | kind2_seed_next_3 >> 8) >> 1 | ((uint32_t)4 + particle_slot_ptr > 0xFFFF) << 15);
                                kind2_seed_next_4 = (uint16_t)((uint16_t)(kind2_seed_rot_4 + kind2_seed_next_3) + kind2_seed_next_3 + ((uint32_t)kind2_seed_rot_4 + kind2_seed_next_3 > 0xFFFF));
                                fx_stb((uint16_t)(particle_slot_ptr + 5), (uint8_t)(uint16_t)((uint16_t)~(0x001F & kind2_seed_next_4) + 1));
                                kind2_seed_rot_5 = (uint16_t)((uint16_t)(kind2_seed_next_4 << 8 | kind2_seed_next_4 >> 8) >> 1 | ((uint32_t)5 + particle_slot_ptr > 0xFFFF) << 15);
                                effect_seed = (uint16_t)((uint16_t)(kind2_seed_rot_5 + kind2_seed_next_4) + kind2_seed_next_4 + ((uint32_t)kind2_seed_rot_5 + kind2_seed_next_4 > 0xFFFF));
                                fx_stb((uint16_t)(particle_slot_ptr + 2), (uint8_t)(uint16_t)((3 & effect_seed) + 0x0068));
                                escape = 1;          /* continue lB763 */
                                break;
                            }
                        } else {
                            escape = 1;          /* continue lB763 */
                            break;
                        }
                    }
                } else {
                    break;
                }
            }
            if (escape == 1) {
                escape = 0;
                continue;
            }
            break;
        }
    }
    fx_stw(0x01D0, effect_seed);
    fx_unspill(stack);
    return (FxEffectsAddNextResult){1};
}


/*
 * @routine 01:B7E6 FxEffectsAddUnused
 * WHAT IT DOES: Code no jump or branch reaches: after kind 6's set-up
 * (which ends by jumping to 01:B763) a second copy of a kind's set-up,
 * its speeds halved differently; nothing in bank 01 jumps or branches
 * here, so a direct call is its only way in.
 * HOW: As kind 6 at 01:B774, ending at 01:B855 with the jump to the
 * next slot (01:B763).
 * Entry: as FxEffectsAdd at a kind's set-up: R3 = the slot, R9 = the
 * particles left, R2 = the seed, R6, R12, R10 = stack. Exit: as
 * FxEffectsAdd.
 */
FxEffectsAddUnusedResult FxEffectsAddUnused(uint16_t seed, uint16_t slot_ptr, uint16_t place_y, uint16_t place_x, uint16_t effect_kind, uint16_t particles_left, uint16_t *stack, uint16_t count)
{
    int escape = 0;                    /* a way out of loops within loops */
    uint16_t all_next;
    uint16_t all_prev;
    uint16_t seed_rot;
    uint16_t seed_next;
    uint16_t all_next_2;
    uint16_t all_prev_2;
    uint16_t frame_byte;
    uint16_t table_addr;
    uint16_t life_offset;
    uint16_t kind6_speed_x;
    uint16_t kind6_speed_y;
    uint16_t kind6_seed_rot;
    uint16_t kind7_seed_rot;
    int kind_at_most_2;
    int kind_at_most_3;
    int kind_at_most_4;
    uint16_t kind2_seed_rot_1;
    uint16_t kind2_seed_rot_2;
    uint16_t kind2_seed_rot_3;
    uint16_t kind2_seed_rot_4;
    uint16_t kind2_seed_rot_5;
    uint16_t kind2_seed_sum_1;
    uint16_t kind3_seed_rot_1;
    uint16_t kind3_seed_rot_2;
    uint16_t kind3_seed_rot_3;
    uint16_t kind3_seed_rot_4;
    uint16_t kind3_seed_rot_5;
    uint16_t kind3_seed_sum_1;
    uint16_t kind4_seed_rot_1;
    uint16_t kind4_seed_rot_2;
    uint16_t kind4_seed_rot_3;
    uint16_t kind4_seed_rot_4;
    uint16_t kind4_seed_rot_5;
    uint16_t kind4_seed_sum_1;
    uint16_t kind6_frame_byte;
    uint16_t kind6_table_addr;
    uint16_t kind7_frame_byte;
    uint16_t kind7_table_addr;
    uint16_t slot_in_use_byte;
    uint16_t kind2_seed_next_1;
    uint16_t kind2_seed_next_2;
    uint16_t kind2_seed_next_3;
    uint16_t kind2_seed_next_4;
    uint16_t kind3_seed_next_1;
    uint16_t kind3_seed_next_2;
    uint16_t kind3_seed_next_3;
    uint16_t kind3_seed_next_4;
    uint16_t kind4_seed_next_1;
    uint16_t kind4_seed_next_2;
    uint16_t kind4_seed_next_3;
    uint16_t kind4_seed_next_4;
    uint16_t kind6_life_offset;
    uint16_t kind7_life_offset;
    uint16_t kind6_speed_x_half;
    uint16_t kind6_speed_y_half;
    uint16_t kind6_count_less_one;
    uint16_t kind7_count_less_one;
    uint16_t particles_left_after;
    uint16_t particles_left_less_one;

    fx_stb(slot_ptr, (uint8_t)(uint16_t)((uint16_t)(place_x + (particles_left & 0x000F)) - 7));
    fx_stb((uint16_t)(slot_ptr + 1), (uint8_t)0x0010);
    particles_left_less_one = (uint16_t)(particles_left - 1);
    table_addr = (uint16_t)((uint16_t)(particles_left_less_one + particles_left_less_one) - 0x2F24);
    fx_r14(table_addr);
    all_prev = (uint16_t)(int16_t)(int8_t)fx_getb();
    fx_r14((uint16_t)(table_addr + 1));
    fx_stb((uint16_t)(slot_ptr + 3), (uint8_t)all_prev);
    fx_stw((uint16_t)(slot_ptr + FXREC_ALL_PREV), all_prev);
    all_next = (uint16_t)(int16_t)(int8_t)fx_getb();
    fx_stb((uint16_t)(slot_ptr + FXREC_ALL_NEXT), (uint8_t)all_next);
    fx_stw((uint16_t)(slot_ptr + FXREC_POOL), all_next);
    seed_rot = (uint16_t)((uint16_t)(seed << 8 | seed >> 8) >> 1 | ((uint32_t)8 + slot_ptr > 0xFFFF) << 15);
    seed_next = (uint16_t)((uint16_t)(seed_rot + seed) + seed + ((uint32_t)seed_rot + seed > 0xFFFF));
    life_offset = (uint16_t)((0x003F & seed_next) - 0x001F);
    fx_stb((uint16_t)(slot_ptr + 5), (uint8_t)life_offset);
    fx_stw((uint16_t)(slot_ptr + FX_PARTICLE_LIFE), life_offset);
    frame_byte = 4;
    if ((1 & count) == 0) {
    } else {
        frame_byte = 0x000E;
    }
    fx_stb((uint16_t)(slot_ptr + 2), (uint8_t)frame_byte);
    for (;;) {
        particles_left_after = (uint16_t)(particles_left - 1);
        particles_left = particles_left_after;
        if (particles_left_after == 0) {
            break;
        } else {
            for (;;) {
                slot_ptr = (uint16_t)(slot_ptr + 0x000E);
                count = (uint16_t)(count - 1);
                if (count != 0) {
                    slot_in_use_byte = fx_ldb(slot_ptr);
                    if ((uint16_t)(slot_in_use_byte & 0xFF) != 0) {
                        continue;
                    } else {
                        fx_stw((uint16_t)(slot_ptr + FXREC_ALL_PREV), 0);
                        fx_stw((uint16_t)(slot_ptr + FXREC_POOL), 0);
                        fx_stw((uint16_t)(slot_ptr + FX_PARTICLE_LIFE), 0);
                        fx_stw((uint16_t)(slot_ptr + 0x000C), place_y);
                        fx_stb(slot_ptr, (uint8_t)place_x);
                        if ((uint16_t)((uint16_t)(0 - effect_kind) + 1) != 0) {
                            kind_at_most_2 = (2 >= effect_kind);
                            if ((uint16_t)((uint16_t)(0 - effect_kind) + 2) != 0) {
                                kind_at_most_3 = (3 >= effect_kind);
                                if ((uint16_t)((uint16_t)(0 - effect_kind) + 3) != 0) {
                                    kind_at_most_4 = (4 >= effect_kind);
                                    if ((uint16_t)((uint16_t)(0 - effect_kind) + 4) != 0) {
                                        if ((uint16_t)((uint16_t)(0 - effect_kind) + 5) != 0) {
                                            if ((uint16_t)((uint16_t)(0 - effect_kind) + 6) != 0) {
                                                if ((uint16_t)((uint16_t)(0 - effect_kind) + 7) != 0) {
                                                    escape = 1;          /* continue lB763 */
                                                    break;
                                                } else {
                                                    fx_stb(slot_ptr, (uint8_t)(uint16_t)((uint16_t)(place_x + (particles_left & 0x000F)) - 7));
                                                    fx_stb((uint16_t)(slot_ptr + 1), (uint8_t)0x0010);
                                                    kind7_count_less_one = (uint16_t)(particles_left - 1);
                                                    kind7_table_addr = (uint16_t)((uint16_t)(kind7_count_less_one + kind7_count_less_one) - 0x2F24);
                                                    fx_r14(kind7_table_addr);
                                                    all_prev_2 = (uint16_t)(int16_t)(int8_t)fx_getb();
                                                    fx_r14((uint16_t)(kind7_table_addr + 1));
                                                    fx_stb((uint16_t)(slot_ptr + 3), (uint8_t)all_prev_2);
                                                    fx_stw((uint16_t)(slot_ptr + FXREC_ALL_PREV), all_prev_2);
                                                    all_next_2 = (uint16_t)(int16_t)(int8_t)fx_getb();
                                                    fx_stb((uint16_t)(slot_ptr + FXREC_ALL_NEXT), (uint8_t)all_next_2);
                                                    fx_stw((uint16_t)(slot_ptr + FXREC_POOL), all_next_2);
                                                    kind7_seed_rot = (uint16_t)((uint16_t)(seed_next << 8 | seed_next >> 8) >> 1 | ((uint32_t)8 + slot_ptr > 0xFFFF) << 15);
                                                    seed_next = (uint16_t)((uint16_t)(kind7_seed_rot + seed_next) + seed_next + ((uint32_t)kind7_seed_rot + seed_next > 0xFFFF));
                                                    kind7_life_offset = (uint16_t)((0x003F & seed_next) - 0x001F);
                                                    fx_stb((uint16_t)(slot_ptr + 5), (uint8_t)kind7_life_offset);
                                                    fx_stw((uint16_t)(slot_ptr + FX_PARTICLE_LIFE), kind7_life_offset);
                                                    kind7_frame_byte = 4;
                                                    if ((1 & count) == 0) {
                                                    } else {
                                                        kind7_frame_byte = 0x000E;
                                                    }
                                                    fx_stb((uint16_t)(slot_ptr + 2), (uint8_t)kind7_frame_byte);
                                                    escape = 1;          /* continue lB763 */
                                                    break;
                                                }
                                            } else {
                                                fx_stb(slot_ptr, (uint8_t)(uint16_t)((uint16_t)(place_x + (particles_left & 0x000F)) - 7));
                                                fx_stb((uint16_t)(slot_ptr + 1), (uint8_t)0x0010);
                                                kind6_count_less_one = (uint16_t)(particles_left - 1);
                                                kind6_table_addr = (uint16_t)((uint16_t)(kind6_count_less_one + kind6_count_less_one) - 0x2F24);
                                                fx_r14(kind6_table_addr);
                                                kind6_speed_x = (uint16_t)(int16_t)(int8_t)fx_getb();
                                                fx_r14((uint16_t)(kind6_table_addr + 1));
                                                kind6_speed_x_half = fx_div2(kind6_speed_x);
                                                fx_stb((uint16_t)(slot_ptr + 3), (uint8_t)kind6_speed_x_half);
                                                fx_stw((uint16_t)(slot_ptr + FXREC_ALL_PREV), kind6_speed_x_half);
                                                kind6_speed_y = (uint16_t)(int16_t)(int8_t)fx_getb();
                                                kind6_speed_y_half = fx_div2(kind6_speed_y);
                                                fx_stb((uint16_t)(slot_ptr + FXREC_ALL_NEXT), (uint8_t)kind6_speed_y_half);
                                                fx_stw((uint16_t)(slot_ptr + FXREC_POOL), kind6_speed_y_half);
                                                kind6_seed_rot = (uint16_t)((uint16_t)(seed_next << 8 | seed_next >> 8) >> 1 | ((uint32_t)8 + slot_ptr > 0xFFFF) << 15);
                                                seed_next = (uint16_t)((uint16_t)(kind6_seed_rot + seed_next) + seed_next + ((uint32_t)kind6_seed_rot + seed_next > 0xFFFF));
                                                kind6_life_offset = (uint16_t)((0x001F & seed_next) - 0x000F);
                                                fx_stb((uint16_t)(slot_ptr + 5), (uint8_t)kind6_life_offset);
                                                fx_stw((uint16_t)(slot_ptr + FX_PARTICLE_LIFE), kind6_life_offset);
                                                kind6_frame_byte = 4;
                                                if ((1 & count) == 0) {
                                                } else {
                                                    kind6_frame_byte = 0x000E;
                                                }
                                                fx_stb((uint16_t)(slot_ptr + 2), (uint8_t)kind6_frame_byte);
                                                escape = 1;          /* continue lB763 */
                                                break;
                                            }
                                        } else {
                                            escape = 1;          /* continue lB763 */
                                            break;
                                        }
                                    } else {
                                        kind4_seed_rot_1 = (uint16_t)((uint16_t)(seed_next << 8 | seed_next >> 8) >> 1 | kind_at_most_4 << 15);
                                        kind4_seed_sum_1 = (uint16_t)(kind4_seed_rot_1 + seed_next);
                                        kind4_seed_next_1 = (uint16_t)(kind4_seed_sum_1 + seed_next + ((uint32_t)kind4_seed_rot_1 + seed_next > 0xFFFF));
                                        fx_stb(slot_ptr, (uint8_t)(uint16_t)(place_x - (kind4_seed_sum_1 & 7)));
                                        fx_stb((uint16_t)(slot_ptr + 1), (uint8_t)4);
                                        kind4_seed_rot_2 = (uint16_t)((uint16_t)(kind4_seed_next_1 << 8 | kind4_seed_next_1 >> 8) >> 1 | ((uint32_t)1 + slot_ptr > 0xFFFF) << 15);
                                        kind4_seed_next_2 = (uint16_t)((uint16_t)(kind4_seed_rot_2 + kind4_seed_next_1) + kind4_seed_next_1 + ((uint32_t)kind4_seed_rot_2 + kind4_seed_next_1 > 0xFFFF));
                                        fx_stb((uint16_t)(slot_ptr + 3), (uint8_t)(uint16_t)((0x000F & kind4_seed_next_2) - 7));
                                        kind4_seed_rot_3 = (uint16_t)((uint16_t)(kind4_seed_next_2 << 8 | kind4_seed_next_2 >> 8) >> 1 | ((uint32_t)3 + slot_ptr > 0xFFFF) << 15);
                                        kind4_seed_next_3 = (uint16_t)((uint16_t)(kind4_seed_rot_3 + kind4_seed_next_2) + kind4_seed_next_2 + ((uint32_t)kind4_seed_rot_3 + kind4_seed_next_2 > 0xFFFF));
                                        fx_stb((uint16_t)(slot_ptr + FXREC_ALL_NEXT), (uint8_t)(uint16_t)((7 & kind4_seed_next_3) - 3));
                                        kind4_seed_rot_4 = (uint16_t)((uint16_t)(kind4_seed_next_3 << 8 | kind4_seed_next_3 >> 8) >> 1 | ((uint32_t)4 + slot_ptr > 0xFFFF) << 15);
                                        kind4_seed_next_4 = (uint16_t)((uint16_t)(kind4_seed_rot_4 + kind4_seed_next_3) + kind4_seed_next_3 + ((uint32_t)kind4_seed_rot_4 + kind4_seed_next_3 > 0xFFFF));
                                        fx_stb((uint16_t)(slot_ptr + 5), (uint8_t)(uint16_t)((uint16_t)~(0x001F & kind4_seed_next_4) + 1));
                                        kind4_seed_rot_5 = (uint16_t)((uint16_t)(kind4_seed_next_4 << 8 | kind4_seed_next_4 >> 8) >> 1 | ((uint32_t)5 + slot_ptr > 0xFFFF) << 15);
                                        seed_next = (uint16_t)((uint16_t)(kind4_seed_rot_5 + kind4_seed_next_4) + kind4_seed_next_4 + ((uint32_t)kind4_seed_rot_5 + kind4_seed_next_4 > 0xFFFF));
                                        fx_stb((uint16_t)(slot_ptr + 2), (uint8_t)(uint16_t)((3 & seed_next) + 0x0068));
                                        escape = 1;          /* continue lB763 */
                                        break;
                                    }
                                } else {
                                    kind3_seed_rot_1 = (uint16_t)((uint16_t)(seed_next << 8 | seed_next >> 8) >> 1 | kind_at_most_3 << 15);
                                    kind3_seed_sum_1 = (uint16_t)(kind3_seed_rot_1 + seed_next);
                                    kind3_seed_next_1 = (uint16_t)(kind3_seed_sum_1 + seed_next + ((uint32_t)kind3_seed_rot_1 + seed_next > 0xFFFF));
                                    fx_stb(slot_ptr, (uint8_t)(uint16_t)(place_x - (kind3_seed_sum_1 & 7)));
                                    fx_stb((uint16_t)(slot_ptr + 1), (uint8_t)0);
                                    kind3_seed_rot_2 = (uint16_t)((uint16_t)(kind3_seed_next_1 << 8 | kind3_seed_next_1 >> 8) >> 1 | ((uint32_t)1 + slot_ptr > 0xFFFF) << 15);
                                    kind3_seed_next_2 = (uint16_t)((uint16_t)(kind3_seed_rot_2 + kind3_seed_next_1) + kind3_seed_next_1 + ((uint32_t)kind3_seed_rot_2 + kind3_seed_next_1 > 0xFFFF));
                                    fx_stb((uint16_t)(slot_ptr + 3), (uint8_t)(uint16_t)((0x000F & kind3_seed_next_2) - 7));
                                    kind3_seed_rot_3 = (uint16_t)((uint16_t)(kind3_seed_next_2 << 8 | kind3_seed_next_2 >> 8) >> 1 | ((uint32_t)3 + slot_ptr > 0xFFFF) << 15);
                                    kind3_seed_next_3 = (uint16_t)((uint16_t)(kind3_seed_rot_3 + kind3_seed_next_2) + kind3_seed_next_2 + ((uint32_t)kind3_seed_rot_3 + kind3_seed_next_2 > 0xFFFF));
                                    fx_stb((uint16_t)(slot_ptr + FXREC_ALL_NEXT), (uint8_t)(uint16_t)((0x000F & kind3_seed_next_3) - 7));
                                    kind3_seed_rot_4 = (uint16_t)((uint16_t)(kind3_seed_next_3 << 8 | kind3_seed_next_3 >> 8) >> 1 | ((uint32_t)4 + slot_ptr > 0xFFFF) << 15);
                                    kind3_seed_next_4 = (uint16_t)((uint16_t)(kind3_seed_rot_4 + kind3_seed_next_3) + kind3_seed_next_3 + ((uint32_t)kind3_seed_rot_4 + kind3_seed_next_3 > 0xFFFF));
                                    fx_stb((uint16_t)(slot_ptr + 5), (uint8_t)(uint16_t)((uint16_t)~(0x007F & kind3_seed_next_4) + 1));
                                    kind3_seed_rot_5 = (uint16_t)((uint16_t)(kind3_seed_next_4 << 8 | kind3_seed_next_4 >> 8) >> 1 | ((uint32_t)5 + slot_ptr > 0xFFFF) << 15);
                                    seed_next = (uint16_t)((uint16_t)(kind3_seed_rot_5 + kind3_seed_next_4) + kind3_seed_next_4 + ((uint32_t)kind3_seed_rot_5 + kind3_seed_next_4 > 0xFFFF));
                                    fx_stb((uint16_t)(slot_ptr + 2), (uint8_t)(uint16_t)((3 & seed_next) + 0x0083));
                                    escape = 1;          /* continue lB763 */
                                    break;
                                }
                            } else {
                                kind2_seed_rot_1 = (uint16_t)((uint16_t)(seed_next << 8 | seed_next >> 8) >> 1 | kind_at_most_2 << 15);
                                kind2_seed_sum_1 = (uint16_t)(kind2_seed_rot_1 + seed_next);
                                kind2_seed_next_1 = (uint16_t)(kind2_seed_sum_1 + seed_next + ((uint32_t)kind2_seed_rot_1 + seed_next > 0xFFFF));
                                fx_stb(slot_ptr, (uint8_t)(uint16_t)(place_x - (kind2_seed_sum_1 & 7)));
                                fx_stb((uint16_t)(slot_ptr + 1), (uint8_t)2);
                                kind2_seed_rot_2 = (uint16_t)((uint16_t)(kind2_seed_next_1 << 8 | kind2_seed_next_1 >> 8) >> 1 | ((uint32_t)1 + slot_ptr > 0xFFFF) << 15);
                                kind2_seed_next_2 = (uint16_t)((uint16_t)(kind2_seed_rot_2 + kind2_seed_next_1) + kind2_seed_next_1 + ((uint32_t)kind2_seed_rot_2 + kind2_seed_next_1 > 0xFFFF));
                                fx_stb((uint16_t)(slot_ptr + 3), (uint8_t)(uint16_t)((0x000F & kind2_seed_next_2) - 7));
                                kind2_seed_rot_3 = (uint16_t)((uint16_t)(kind2_seed_next_2 << 8 | kind2_seed_next_2 >> 8) >> 1 | ((uint32_t)3 + slot_ptr > 0xFFFF) << 15);
                                kind2_seed_next_3 = (uint16_t)((uint16_t)(kind2_seed_rot_3 + kind2_seed_next_2) + kind2_seed_next_2 + ((uint32_t)kind2_seed_rot_3 + kind2_seed_next_2 > 0xFFFF));
                                fx_stb((uint16_t)(slot_ptr + FXREC_ALL_NEXT), (uint8_t)(uint16_t)((7 & kind2_seed_next_3) - 3));
                                kind2_seed_rot_4 = (uint16_t)((uint16_t)(kind2_seed_next_3 << 8 | kind2_seed_next_3 >> 8) >> 1 | ((uint32_t)4 + slot_ptr > 0xFFFF) << 15);
                                kind2_seed_next_4 = (uint16_t)((uint16_t)(kind2_seed_rot_4 + kind2_seed_next_3) + kind2_seed_next_3 + ((uint32_t)kind2_seed_rot_4 + kind2_seed_next_3 > 0xFFFF));
                                fx_stb((uint16_t)(slot_ptr + 5), (uint8_t)(uint16_t)((uint16_t)~(0x001F & kind2_seed_next_4) + 1));
                                kind2_seed_rot_5 = (uint16_t)((uint16_t)(kind2_seed_next_4 << 8 | kind2_seed_next_4 >> 8) >> 1 | ((uint32_t)5 + slot_ptr > 0xFFFF) << 15);
                                seed_next = (uint16_t)((uint16_t)(kind2_seed_rot_5 + kind2_seed_next_4) + kind2_seed_next_4 + ((uint32_t)kind2_seed_rot_5 + kind2_seed_next_4 > 0xFFFF));
                                fx_stb((uint16_t)(slot_ptr + 2), (uint8_t)(uint16_t)((3 & seed_next) + 0x0068));
                                escape = 1;          /* continue lB763 */
                                break;
                            }
                        } else {
                            escape = 1;          /* continue lB763 */
                            break;
                        }
                    }
                } else {
                    break;
                }
            }
            if (escape == 1) {
                escape = 0;
                continue;
            }
            break;
        }
    }
    fx_stw(0x01D0, seed_next);
    fx_unspill(stack);
    return (FxEffectsAddUnusedResult){1};
}


/*
 * @routine 01:B856 FxEffectsAddKinds
 * WHAT IT DOES: FxEffectsAdd's set-ups for kinds 3, 4, 5 and 7
 * (01:B856, 01:B92F, 01:B99B, 01:B9A3) and kind 1's (01:B99F): each
 * particle's colour, life and speeds from the seed, then the next slot
 * (01:B763).
 * HOW: Reached by jump from FxEffectsAdd's dispatch on the kind; the
 * seed stepped as SWAP, ROR, ADD, ADC.
 * Entry: as FxEffectsAdd leaves them at the dispatch.
 * Exit: as FxEffectsAdd.
 */
FxEffectsAddKindsResult FxEffectsAddKinds(uint16_t seed, uint16_t particle_ptr, uint16_t place_y, uint16_t base_color, uint16_t effect_kind, uint16_t effect_kind_2, uint16_t *stack, uint16_t count, int rC)
{
    int escape = 0;                    /* a way out of loops within loops */
    uint16_t seed_1;
    uint16_t seed_2;
    uint16_t seed_3;
    uint16_t seed_4;
    uint16_t all_next;
    uint16_t all_prev;
    uint16_t seed_mix_1;
    uint16_t kind6_speed_x;
    uint16_t kind6_speed_y;
    uint16_t kind6_seed_rot;
    uint16_t kind7_seed_rot;
    int kind_at_most_2;
    int kind_at_most_4;
    uint16_t seed_swapped_1;
    uint16_t seed_swapped_2;
    uint16_t seed_swapped_3;
    uint16_t seed_swapped_4;
    uint16_t seed_swapped_5;
    uint16_t kind2_seed_rot_1;
    uint16_t kind2_seed_rot_2;
    uint16_t kind2_seed_rot_3;
    uint16_t kind2_seed_rot_4;
    uint16_t kind2_seed_rot_5;
    uint16_t kind2_seed_sum_1;
    uint16_t kind4_seed_rot_1;
    uint16_t kind4_seed_rot_2;
    uint16_t kind4_seed_rot_3;
    uint16_t kind4_seed_rot_4;
    uint16_t kind4_seed_rot_5;
    uint16_t kind4_seed_sum_1;
    uint16_t kind6_frame_byte;
    uint16_t kind6_table_addr;
    uint16_t kind7_frame_byte;
    uint16_t kind7_table_addr;
    uint16_t slot_in_use_byte;
    uint16_t kind2_seed_next_1;
    uint16_t kind2_seed_next_2;
    uint16_t kind2_seed_next_3;
    uint16_t kind2_seed_next_4;
    uint16_t kind4_seed_next_1;
    uint16_t kind4_seed_next_2;
    uint16_t kind4_seed_next_3;
    uint16_t kind4_seed_next_4;
    uint16_t kind6_life_offset;
    uint16_t kind7_life_offset;
    uint16_t kind6_speed_x_half;
    uint16_t kind6_speed_y_half;
    uint16_t kind6_count_less_one;
    uint16_t kind7_count_less_one;
    uint16_t particles_left_after;

    for (;;) {
        seed_swapped_1 = (uint16_t)((uint16_t)(seed << 8 | seed >> 8) >> 1 | rC << 15);
        seed_mix_1 = (uint16_t)(seed_swapped_1 + seed);
        seed_1 = (uint16_t)(seed_mix_1 + seed + ((uint32_t)seed_swapped_1 + seed > 0xFFFF));
        fx_stb(particle_ptr, (uint8_t)(uint16_t)(base_color - (seed_mix_1 & 7)));
        fx_stb((uint16_t)(particle_ptr + 1), (uint8_t)0);
        seed_swapped_2 = (uint16_t)((uint16_t)(seed_1 << 8 | seed_1 >> 8) >> 1 | ((uint32_t)1 + particle_ptr > 0xFFFF) << 15);
        seed_2 = (uint16_t)((uint16_t)(seed_swapped_2 + seed_1) + seed_1 + ((uint32_t)seed_swapped_2 + seed_1 > 0xFFFF));
        fx_stb((uint16_t)(particle_ptr + 3), (uint8_t)(uint16_t)((0x000F & seed_2) - 7));
        seed_swapped_3 = (uint16_t)((uint16_t)(seed_2 << 8 | seed_2 >> 8) >> 1 | ((uint32_t)3 + particle_ptr > 0xFFFF) << 15);
        seed_3 = (uint16_t)((uint16_t)(seed_swapped_3 + seed_2) + seed_2 + ((uint32_t)seed_swapped_3 + seed_2 > 0xFFFF));
        fx_stb((uint16_t)(particle_ptr + FXREC_ALL_NEXT), (uint8_t)(uint16_t)((0x000F & seed_3) - 7));
        seed_swapped_4 = (uint16_t)((uint16_t)(seed_3 << 8 | seed_3 >> 8) >> 1 | ((uint32_t)4 + particle_ptr > 0xFFFF) << 15);
        seed_4 = (uint16_t)((uint16_t)(seed_swapped_4 + seed_3) + seed_3 + ((uint32_t)seed_swapped_4 + seed_3 > 0xFFFF));
        fx_stb((uint16_t)(particle_ptr + 5), (uint8_t)(uint16_t)((uint16_t)~(0x007F & seed_4) + 1));
        seed_swapped_5 = (uint16_t)((uint16_t)(seed_4 << 8 | seed_4 >> 8) >> 1 | ((uint32_t)5 + particle_ptr > 0xFFFF) << 15);
        seed = (uint16_t)((uint16_t)(seed_swapped_5 + seed_4) + seed_4 + ((uint32_t)seed_swapped_5 + seed_4 > 0xFFFF));
        fx_stb((uint16_t)(particle_ptr + 2), (uint8_t)(uint16_t)((3 & seed) + 0x0083));
        for (;;) {
            particles_left_after = (uint16_t)(effect_kind_2 - 1);
            effect_kind_2 = particles_left_after;
            if (particles_left_after == 0) {
                break;
            } else {
                for (;;) {
                    particle_ptr = (uint16_t)(particle_ptr + 0x000E);
                    count = (uint16_t)(count - 1);
                    if (count != 0) {
                        slot_in_use_byte = fx_ldb(particle_ptr);
                        if ((uint16_t)(slot_in_use_byte & 0xFF) != 0) {
                            continue;
                        } else {
                            fx_stw((uint16_t)(particle_ptr + FXREC_ALL_PREV), 0);
                            fx_stw((uint16_t)(particle_ptr + FXREC_POOL), 0);
                            fx_stw((uint16_t)(particle_ptr + FX_PARTICLE_LIFE), 0);
                            fx_stw((uint16_t)(particle_ptr + 0x000C), place_y);
                            fx_stb(particle_ptr, (uint8_t)base_color);
                            if ((uint16_t)((uint16_t)(0 - effect_kind) + 1) != 0) {
                                kind_at_most_2 = (2 >= effect_kind);
                                if ((uint16_t)((uint16_t)(0 - effect_kind) + 2) != 0) {
                                    rC = (3 >= effect_kind);
                                    if ((uint16_t)((uint16_t)(0 - effect_kind) + 3) != 0) {
                                        kind_at_most_4 = (4 >= effect_kind);
                                        if ((uint16_t)((uint16_t)(0 - effect_kind) + 4) != 0) {
                                            if ((uint16_t)((uint16_t)(0 - effect_kind) + 5) != 0) {
                                                if ((uint16_t)((uint16_t)(0 - effect_kind) + 6) != 0) {
                                                    if ((uint16_t)((uint16_t)(0 - effect_kind) + 7) != 0) {
                                                        escape = 1;          /* continue lB763 */
                                                        break;
                                                    } else {
                                                        fx_stb(particle_ptr, (uint8_t)(uint16_t)((uint16_t)(base_color + (effect_kind_2 & 0x000F)) - 7));
                                                        fx_stb((uint16_t)(particle_ptr + 1), (uint8_t)0x0010);
                                                        kind7_count_less_one = (uint16_t)(effect_kind_2 - 1);
                                                        kind7_table_addr = (uint16_t)((uint16_t)(kind7_count_less_one + kind7_count_less_one) - 0x2F24);
                                                        fx_r14(kind7_table_addr);
                                                        all_prev = (uint16_t)(int16_t)(int8_t)fx_getb();
                                                        fx_r14((uint16_t)(kind7_table_addr + 1));
                                                        fx_stb((uint16_t)(particle_ptr + 3), (uint8_t)all_prev);
                                                        fx_stw((uint16_t)(particle_ptr + FXREC_ALL_PREV), all_prev);
                                                        all_next = (uint16_t)(int16_t)(int8_t)fx_getb();
                                                        fx_stb((uint16_t)(particle_ptr + FXREC_ALL_NEXT), (uint8_t)all_next);
                                                        fx_stw((uint16_t)(particle_ptr + FXREC_POOL), all_next);
                                                        kind7_seed_rot = (uint16_t)((uint16_t)(seed << 8 | seed >> 8) >> 1 | ((uint32_t)8 + particle_ptr > 0xFFFF) << 15);
                                                        seed = (uint16_t)((uint16_t)(kind7_seed_rot + seed) + seed + ((uint32_t)kind7_seed_rot + seed > 0xFFFF));
                                                        kind7_life_offset = (uint16_t)((0x003F & seed) - 0x001F);
                                                        fx_stb((uint16_t)(particle_ptr + 5), (uint8_t)kind7_life_offset);
                                                        fx_stw((uint16_t)(particle_ptr + FX_PARTICLE_LIFE), kind7_life_offset);
                                                        kind7_frame_byte = 4;
                                                        if ((1 & count) == 0) {
                                                        } else {
                                                            kind7_frame_byte = 0x000E;
                                                        }
                                                        fx_stb((uint16_t)(particle_ptr + 2), (uint8_t)kind7_frame_byte);
                                                        escape = 1;          /* continue lB763 */
                                                        break;
                                                    }
                                                } else {
                                                    fx_stb(particle_ptr, (uint8_t)(uint16_t)((uint16_t)(base_color + (effect_kind_2 & 0x000F)) - 7));
                                                    fx_stb((uint16_t)(particle_ptr + 1), (uint8_t)0x0010);
                                                    kind6_count_less_one = (uint16_t)(effect_kind_2 - 1);
                                                    kind6_table_addr = (uint16_t)((uint16_t)(kind6_count_less_one + kind6_count_less_one) - 0x2F24);
                                                    fx_r14(kind6_table_addr);
                                                    kind6_speed_x = (uint16_t)(int16_t)(int8_t)fx_getb();
                                                    fx_r14((uint16_t)(kind6_table_addr + 1));
                                                    kind6_speed_x_half = fx_div2(kind6_speed_x);
                                                    fx_stb((uint16_t)(particle_ptr + 3), (uint8_t)kind6_speed_x_half);
                                                    fx_stw((uint16_t)(particle_ptr + FXREC_ALL_PREV), kind6_speed_x_half);
                                                    kind6_speed_y = (uint16_t)(int16_t)(int8_t)fx_getb();
                                                    kind6_speed_y_half = fx_div2(kind6_speed_y);
                                                    fx_stb((uint16_t)(particle_ptr + FXREC_ALL_NEXT), (uint8_t)kind6_speed_y_half);
                                                    fx_stw((uint16_t)(particle_ptr + FXREC_POOL), kind6_speed_y_half);
                                                    kind6_seed_rot = (uint16_t)((uint16_t)(seed << 8 | seed >> 8) >> 1 | ((uint32_t)8 + particle_ptr > 0xFFFF) << 15);
                                                    seed = (uint16_t)((uint16_t)(kind6_seed_rot + seed) + seed + ((uint32_t)kind6_seed_rot + seed > 0xFFFF));
                                                    kind6_life_offset = (uint16_t)((0x001F & seed) - 0x000F);
                                                    fx_stb((uint16_t)(particle_ptr + 5), (uint8_t)kind6_life_offset);
                                                    fx_stw((uint16_t)(particle_ptr + FX_PARTICLE_LIFE), kind6_life_offset);
                                                    kind6_frame_byte = 4;
                                                    if ((1 & count) == 0) {
                                                    } else {
                                                        kind6_frame_byte = 0x000E;
                                                    }
                                                    fx_stb((uint16_t)(particle_ptr + 2), (uint8_t)kind6_frame_byte);
                                                    escape = 1;          /* continue lB763 */
                                                    break;
                                                }
                                            } else {
                                                escape = 1;          /* continue lB763 */
                                                break;
                                            }
                                        } else {
                                            kind4_seed_rot_1 = (uint16_t)((uint16_t)(seed << 8 | seed >> 8) >> 1 | kind_at_most_4 << 15);
                                            kind4_seed_sum_1 = (uint16_t)(kind4_seed_rot_1 + seed);
                                            kind4_seed_next_1 = (uint16_t)(kind4_seed_sum_1 + seed + ((uint32_t)kind4_seed_rot_1 + seed > 0xFFFF));
                                            fx_stb(particle_ptr, (uint8_t)(uint16_t)(base_color - (kind4_seed_sum_1 & 7)));
                                            fx_stb((uint16_t)(particle_ptr + 1), (uint8_t)4);
                                            kind4_seed_rot_2 = (uint16_t)((uint16_t)(kind4_seed_next_1 << 8 | kind4_seed_next_1 >> 8) >> 1 | ((uint32_t)1 + particle_ptr > 0xFFFF) << 15);
                                            kind4_seed_next_2 = (uint16_t)((uint16_t)(kind4_seed_rot_2 + kind4_seed_next_1) + kind4_seed_next_1 + ((uint32_t)kind4_seed_rot_2 + kind4_seed_next_1 > 0xFFFF));
                                            fx_stb((uint16_t)(particle_ptr + 3), (uint8_t)(uint16_t)((0x000F & kind4_seed_next_2) - 7));
                                            kind4_seed_rot_3 = (uint16_t)((uint16_t)(kind4_seed_next_2 << 8 | kind4_seed_next_2 >> 8) >> 1 | ((uint32_t)3 + particle_ptr > 0xFFFF) << 15);
                                            kind4_seed_next_3 = (uint16_t)((uint16_t)(kind4_seed_rot_3 + kind4_seed_next_2) + kind4_seed_next_2 + ((uint32_t)kind4_seed_rot_3 + kind4_seed_next_2 > 0xFFFF));
                                            fx_stb((uint16_t)(particle_ptr + FXREC_ALL_NEXT), (uint8_t)(uint16_t)((7 & kind4_seed_next_3) - 3));
                                            kind4_seed_rot_4 = (uint16_t)((uint16_t)(kind4_seed_next_3 << 8 | kind4_seed_next_3 >> 8) >> 1 | ((uint32_t)4 + particle_ptr > 0xFFFF) << 15);
                                            kind4_seed_next_4 = (uint16_t)((uint16_t)(kind4_seed_rot_4 + kind4_seed_next_3) + kind4_seed_next_3 + ((uint32_t)kind4_seed_rot_4 + kind4_seed_next_3 > 0xFFFF));
                                            fx_stb((uint16_t)(particle_ptr + 5), (uint8_t)(uint16_t)((uint16_t)~(0x001F & kind4_seed_next_4) + 1));
                                            kind4_seed_rot_5 = (uint16_t)((uint16_t)(kind4_seed_next_4 << 8 | kind4_seed_next_4 >> 8) >> 1 | ((uint32_t)5 + particle_ptr > 0xFFFF) << 15);
                                            seed = (uint16_t)((uint16_t)(kind4_seed_rot_5 + kind4_seed_next_4) + kind4_seed_next_4 + ((uint32_t)kind4_seed_rot_5 + kind4_seed_next_4 > 0xFFFF));
                                            fx_stb((uint16_t)(particle_ptr + 2), (uint8_t)(uint16_t)((3 & seed) + 0x0068));
                                            escape = 1;          /* continue lB763 */
                                            break;
                                        }
                                    } else {
                                        escape = 2;          /* continue lB856 */
                                        break;
                                    }
                                } else {
                                    kind2_seed_rot_1 = (uint16_t)((uint16_t)(seed << 8 | seed >> 8) >> 1 | kind_at_most_2 << 15);
                                    kind2_seed_sum_1 = (uint16_t)(kind2_seed_rot_1 + seed);
                                    kind2_seed_next_1 = (uint16_t)(kind2_seed_sum_1 + seed + ((uint32_t)kind2_seed_rot_1 + seed > 0xFFFF));
                                    fx_stb(particle_ptr, (uint8_t)(uint16_t)(base_color - (kind2_seed_sum_1 & 7)));
                                    fx_stb((uint16_t)(particle_ptr + 1), (uint8_t)2);
                                    kind2_seed_rot_2 = (uint16_t)((uint16_t)(kind2_seed_next_1 << 8 | kind2_seed_next_1 >> 8) >> 1 | ((uint32_t)1 + particle_ptr > 0xFFFF) << 15);
                                    kind2_seed_next_2 = (uint16_t)((uint16_t)(kind2_seed_rot_2 + kind2_seed_next_1) + kind2_seed_next_1 + ((uint32_t)kind2_seed_rot_2 + kind2_seed_next_1 > 0xFFFF));
                                    fx_stb((uint16_t)(particle_ptr + 3), (uint8_t)(uint16_t)((0x000F & kind2_seed_next_2) - 7));
                                    kind2_seed_rot_3 = (uint16_t)((uint16_t)(kind2_seed_next_2 << 8 | kind2_seed_next_2 >> 8) >> 1 | ((uint32_t)3 + particle_ptr > 0xFFFF) << 15);
                                    kind2_seed_next_3 = (uint16_t)((uint16_t)(kind2_seed_rot_3 + kind2_seed_next_2) + kind2_seed_next_2 + ((uint32_t)kind2_seed_rot_3 + kind2_seed_next_2 > 0xFFFF));
                                    fx_stb((uint16_t)(particle_ptr + FXREC_ALL_NEXT), (uint8_t)(uint16_t)((7 & kind2_seed_next_3) - 3));
                                    kind2_seed_rot_4 = (uint16_t)((uint16_t)(kind2_seed_next_3 << 8 | kind2_seed_next_3 >> 8) >> 1 | ((uint32_t)4 + particle_ptr > 0xFFFF) << 15);
                                    kind2_seed_next_4 = (uint16_t)((uint16_t)(kind2_seed_rot_4 + kind2_seed_next_3) + kind2_seed_next_3 + ((uint32_t)kind2_seed_rot_4 + kind2_seed_next_3 > 0xFFFF));
                                    fx_stb((uint16_t)(particle_ptr + 5), (uint8_t)(uint16_t)((uint16_t)~(0x001F & kind2_seed_next_4) + 1));
                                    kind2_seed_rot_5 = (uint16_t)((uint16_t)(kind2_seed_next_4 << 8 | kind2_seed_next_4 >> 8) >> 1 | ((uint32_t)5 + particle_ptr > 0xFFFF) << 15);
                                    seed = (uint16_t)((uint16_t)(kind2_seed_rot_5 + kind2_seed_next_4) + kind2_seed_next_4 + ((uint32_t)kind2_seed_rot_5 + kind2_seed_next_4 > 0xFFFF));
                                    fx_stb((uint16_t)(particle_ptr + 2), (uint8_t)(uint16_t)((3 & seed) + 0x0068));
                                    escape = 1;          /* continue lB763 */
                                    break;
                                }
                            } else {
                                escape = 1;          /* continue lB763 */
                                break;
                            }
                        }
                    } else {
                        break;
                    }
                }
                if (escape == 1) {
                    escape = 0;
                    continue;
                }
                if (escape == 2)
                    break;
                break;
            }
        }
        if (escape == 2) {
            escape = 0;
            continue;
        }
        break;
    }
    fx_stw(0x01D0, seed);
    fx_unspill(stack);
    return (FxEffectsAddKindsResult){1};
}


/*
 * @routine 01:BC35 FxWeatherReset
 * WHAT IT DOES: Sets the weather afresh: $2B94 = R0 (the weather now
 * drawn), the 80 drops at $2864 (ten bytes each: a height from 0 down
 * by one, then nine zeros), $2B8E and $2B90 cleared and the 48 words at
 * $2688 cleared.
 * HOW: Pushes R11 on the R10 stack; CACHE; two LOOPs of STB and one of
 * STW; returns by pulling R15. FxWeatherDraw branches here (01:BC83)
 * when R0 is not $2B94.
 * Entry: R0 = the weather; R10 = stack; R11 = return address.
 * Exit: R0 = 0; R1 past the words at $2688; R2 = 0; R12 = 0; R13 the
 * last loop; S and Z from R10; the last RAM address R10.
 */
uint16_t FxWeatherReset(uint16_t weather, uint16_t *stack)
{
    uint16_t count;
    uint16_t count_2;
    uint16_t drop_ptr;
    uint16_t table_ptr;
    uint16_t drop_count;
    uint16_t drop_phase;
    uint16_t drops_left;

    fx_stw(FX_INIT_2B94, weather);
    fx_spill(stack);
    drop_count = 0x0050;
    drop_ptr = 0x2864;
    drop_phase = 0;
    for (;;) {
        fx_stb(drop_ptr, (uint8_t)(uint16_t)((uint16_t)~drop_phase + 1));
        drop_ptr = (uint16_t)(drop_ptr + 1);
        drop_phase = ((uint16_t)(drop_phase + 1) & 0x003F);
        count = 9;
        for (;;) {
            fx_stb(drop_ptr, (uint8_t)0);
            count = (uint16_t)(count - 1);
            drop_ptr = (uint16_t)(drop_ptr + 1);
            if (count != 0) {
                continue;
            } else {
                break;
            }
        }
        drops_left = (uint16_t)(drop_count - 1);
        drop_count = drops_left;
        if (drops_left != 0) {
            continue;
        } else {
            break;
        }
    }
    fx_stw(FX_WEATHER_TURN, 0);
    fx_stw(FX_WEATHER_DRIFT, 0);
    table_ptr = 0x2688;
    count_2 = 0x0030;
    for (;;) {
        fx_stw(table_ptr, 0);
        table_ptr = (uint16_t)(table_ptr + 2);
        count_2 = (uint16_t)(count_2 - 1);
        if (count_2 != 0) {
            continue;
        } else {
            break;
        }
    }
    fx_unspill(stack);
    return drop_count;
}


/*
 * @routine 01:BC7D FxWeatherDraw
 * WHAT IT DOES: Draws the weather: R0 = 1 (rain) or 2 (snow) with R1
 * drops; when R0 is not $2B94 the drops are set afresh instead
 * (FxWeatherReset); each drop moved by the view's turn ($00E0) and
 * drawn with PLOT.
 * HOW: Pushes R11 on the R10 stack; RAMB 0, ROMB $14, CMODE 1; the drop
 * list at $2864; the seed at $01D0; returns by pulling R15.
 * Entry: R0 = the weather, R1 = the drops; R10 = stack; R11 = return
 * address.
 * Exit: $01D0 = the seed; the rest as the code leaves them.
 */
uint16_t FxWeatherDraw(uint16_t weather, uint16_t drops, uint16_t plot_y, uint16_t *stack)
{
    int run_lBC97 = 0;
    int run_lBDDD = 0;
    int run_lBE0C = 0;
    uint16_t dx;
    uint16_t dy;
    uint16_t me_z;
    uint16_t count;
    uint16_t speed;
    uint16_t drop_x;
    uint16_t drop_y;
    uint16_t drop_z;
    uint16_t dx_abs;
    uint16_t dy_abs;
    uint16_t x_step;
    uint16_t y_step;
    uint16_t count_2;
    uint16_t count_3;
    uint16_t haze_x8;
    uint16_t clip_top;
    uint16_t drop_ptr;
    uint16_t new_life;
    uint16_t screen_x;
    uint16_t seed_sum;
    uint16_t clip_left;
    uint16_t drop_life;
    uint16_t haze_x8_2;
    uint16_t init_2b94;
    uint16_t loop_addr;
    uint16_t seed_mask;
    uint16_t clip_right;
    uint16_t drift_bias;
    uint16_t drift_mask;
    uint16_t drop_x_ptr;
    uint16_t drop_y_ptr;
    uint16_t drop_z_ptr;
    uint16_t new_drop_z;
    uint16_t param_00e0;
    uint16_t prev_y_ptr;
    int seed_carry;
    uint16_t seed_sum_2;
    uint16_t seed_sum_3;
    uint16_t clip_bottom;
    uint16_t drop_colour;
    uint16_t dx_minus_dy;
    uint16_t effect_seed;
    uint16_t recip_scale;
    uint16_t seed_next_1;
    uint16_t drop_x_moved;
    uint16_t drop_y_moved;
    uint16_t drop_z_bound;
    uint16_t drop_z_moved;
    int seed_carry_2;
    uint16_t seed_rotated;
    uint16_t weather_turn;
    uint16_t drift_x_delta;
    uint16_t drop_life_ptr;
    uint16_t drop_life_raw;
    uint16_t drop_x_biased;
    uint16_t drop_y_biased;
    uint16_t me_record_ptr;
    uint16_t prev_screen_y;
    uint16_t screen_x_back;
    uint16_t screen_x_next;
    uint32_t screen_x_prod;
    uint32_t screen_y_prod;
    uint16_t view_center_x;
    uint16_t view_center_y;
    uint16_t weather_drift;
    uint16_t approach_speed;
    uint16_t drop_extra_ptr;
    uint16_t drop_x_clamped;
    uint16_t drop_x_ptr_new;
    uint16_t drop_x_wrapped;
    uint16_t drop_y_clamped;
    uint16_t drop_y_ptr_new;
    uint16_t drop_y_wrapped;
    uint16_t drop_z_ptr_new;
    uint16_t recip_low_byte;
    uint16_t seed_rotated_2;
    uint16_t seed_rotated_3;
    uint16_t weather_colour;
    uint16_t drop_colour_ptr;
    uint16_t drop_x_over_max;
    uint16_t drop_y_over_max;
    uint16_t prev_screen_pos;
    uint16_t weather_drift_2;
    int weather_ge_rain;
    uint16_t drop_x_low_bound;
    uint16_t drop_y_low_bound;
    uint16_t half_dx_minus_dy;
    uint16_t half_dy_minus_dx;
    uint16_t line_error_steep;
    uint16_t recip_table_addr;
    uint16_t perspective_index;
    uint16_t view_center_y_new;
    uint16_t drop_extra_ptr_new;
    uint16_t drop_life_less_one;
    uint16_t line_error_shallow;
    uint16_t x_after_plot_steep;
    uint16_t drop_colour_ptr_move;
    uint16_t x_after_plot_shallow;
    uint16_t drop_z_ptr_after_x_reset;
    uint16_t drop_extra_ptr_after_y_reset;

    init_2b94 = fx_ldw(FX_INIT_2B94);
    if (weather != init_2b94) {
        return FxWeatherReset(weather, stack);
    }
    if (weather == 1) {
        run_lBC97 = 1;
    } else {
        if (weather == 2) {
            run_lBC97 = 1;
        } else {
            return plot_y;
        }
    }
    fx_spill(stack);
    fx_weather_begin(weather);          /* the drops drawn at the window's pixels and across the view (fx_weather.c) */
    effect_seed = fx_ldw(0x01D0);
    count = drops;
    if (drops != 0) {
        fx_ramb(0);
        fx_romb(0x0014);
        fx_cmode(1);
        weather_ge_rain = (weather >= 1);
        if (weather == 1) {
            seed_mask = 0x001F;
            drift_mask = 0x003F;
            drift_bias = 0x001F;
            haze_x8 = 0x0028;
            fx_stw(FX_WEATHER_COLOUR, 0x002A);
            fx_stw(0x26FC, 0x001E);
        } else {
            seed_mask = 0x001F;
            drift_mask = 0x000F;
            drift_bias = 7;
            haze_x8 = 5;
            fx_stw(FX_WEATHER_COLOUR, 0x0020);
            fx_stw(0x26FC, 0x0064);
        }
        plot_y = fx_ldw(FX_WEATHER_DRIFT);
        seed_rotated = (uint16_t)((uint16_t)(effect_seed << 8 | effect_seed >> 8) >> 1 | weather_ge_rain << 15);
        seed_sum = (uint16_t)(seed_rotated + effect_seed);
        effect_seed = (uint16_t)(seed_sum + effect_seed + ((uint32_t)seed_rotated + effect_seed > 0xFFFF));
        if ((seed_sum & seed_mask) != 0) {
        } else {
            plot_y = (uint16_t)(((uint16_t)(seed_sum >> 8) & drift_mask) - drift_bias);
        }
        weather_drift = fx_ldw(FX_WEATHER_DRIFT);
        fx_sbk((uint16_t)(fx_div2(fx_div2(fx_div2((uint16_t)(plot_y - weather_drift)))) + weather_drift));
        param_00e0 = fx_ldw(GSU_PARAM_00E0);
        weather_turn = fx_ldw(FX_WEATHER_TURN);
        fx_stw(FX_WEATHER_TURN, param_00e0);
        weather_drift_2 = fx_ldw(FX_WEATHER_DRIFT);
        fx_stw(0x26F6, (uint16_t)((uint16_t)((uint16_t)~fx_div2(fx_div2(fx_div2(fx_div2(fx_div2((uint16_t)(param_00e0 - weather_turn)))))) + weather_drift_2) + 1));
        fx_stw(FX_HAZE_X8, haze_x8);
        me_record_ptr = fx_ldw(0x247A);
        speed = fx_ldw((uint16_t)(me_record_ptr + FXREC_SPEED));
        if (!(speed & 0x8000)) {
            approach_speed = fx_div2(fx_div2((uint16_t)((uint16_t)~speed + 1)));
            seed_carry = (approach_speed & 1);
            fx_stw(FX_ME_Z, fx_div2(approach_speed));
            drop_ptr = 0x2864;
            loop_addr = 0xBD52;
            for (;;) {
                drop_life_raw = fx_ldb(drop_ptr);
                drop_life = (uint16_t)(int16_t)(int8_t)drop_life_raw;
                drop_life_less_one = (uint16_t)(drop_life - 1);
                if (drop_life == 0) {
                    new_life = fx_ldw(0x26FC);
                    fx_stb(drop_ptr, (uint8_t)new_life);
                    drop_colour_ptr = (uint16_t)(drop_ptr + 1);
                    seed_rotated_2 = (uint16_t)((uint16_t)(effect_seed << 8 | effect_seed >> 8) >> 1 | seed_carry << 15);
                    seed_sum_2 = (uint16_t)(seed_rotated_2 + effect_seed);
                    seed_carry_2 = ((uint32_t)seed_rotated_2 + effect_seed > 0xFFFF);
                    seed_next_1 = (uint16_t)(seed_sum_2 + effect_seed + seed_carry_2);
                    weather_colour = fx_ldw(FX_WEATHER_COLOUR);
                    fx_stb(drop_colour_ptr, (uint8_t)weather_colour);
                    drop_x_ptr_new = (uint16_t)(drop_colour_ptr + 1);
                    new_drop_z = (weather_colour & 0x01FF);
                    seed_rotated_3 = (uint16_t)((uint16_t)(seed_next_1 << 8 | seed_next_1 >> 8) >> 1 | ((uint32_t)seed_sum_2 + effect_seed + seed_carry_2 > 0xFFFF) << 15);
                    seed_sum_3 = (uint16_t)(seed_rotated_3 + seed_next_1);
                    effect_seed = (uint16_t)(seed_sum_3 + seed_next_1 + ((uint32_t)seed_rotated_3 + seed_next_1 > 0xFFFF));
                    fx_stw(drop_x_ptr_new, (uint16_t)((seed_sum_3 & 0x03FF) - 0x01FF));
                    drop_y_ptr_new = (uint16_t)(drop_x_ptr_new + 2);
                    view_center_y_new = fx_ldw(0x0036);
                    fx_stw(drop_y_ptr_new, (uint16_t)((uint16_t)~(uint16_t)(fx_div2(new_drop_z) + view_center_y_new) + 1));
                    drop_z_ptr_new = (uint16_t)(drop_y_ptr_new + 2);
                    fx_stw(drop_z_ptr_new, new_drop_z);
                    drop_extra_ptr_new = (uint16_t)(drop_z_ptr_new + 2);
                    seed_carry = 1;
                    fx_stw(drop_extra_ptr_new, 0);
                    drop_ptr = (uint16_t)(drop_extra_ptr_new + 2);
                } else {
                    if (!(drop_life_less_one & 0x8000)) {
                        fx_stb(drop_ptr, (uint8_t)drop_life_less_one);
                        drop_colour_ptr_move = (uint16_t)(drop_ptr + 1);
                        drop_colour = fx_ldb(drop_colour_ptr_move);
                        drop_x_ptr = (uint16_t)(drop_colour_ptr_move + 1);
                        fx_color((uint8_t)drop_colour);
                        drift_x_delta = fx_ldw(0x26F6);
                        drop_x = fx_ldw(drop_x_ptr);
                        drop_y_ptr = (uint16_t)(drop_x_ptr + 2);
                        drop_x_moved = (uint16_t)(drop_x + drift_x_delta);
                        if (!(drop_x_moved & 0x8000)) {
                            drop_x_over_max = (uint16_t)(drop_x_moved - 0x01FF);
                            drop_x_wrapped = (uint16_t)(drop_x_over_max + 0x01FF);
                            if (drop_x_over_max & 0x8000) {
                                run_lBDDD = 1;
                            } else {
                                drop_x_low_bound = (uint16_t)((uint16_t)~0x01FF + 1);
                                fx_sbk(drop_x_low_bound);
                                drop_z_ptr_after_x_reset = (uint16_t)(drop_y_ptr + 4);
                                seed_carry = 1;
                                fx_stw(drop_z_ptr_after_x_reset, 0);
                                drop_ptr = (uint16_t)(drop_z_ptr_after_x_reset + 2);
                            }
                        } else {
                            drop_x_biased = (uint16_t)(drop_x_moved + 0x01FF);
                            drop_x_wrapped = (uint16_t)(drop_x_biased - 0x01FF);
                            if (!(drop_x_biased & 0x8000)) {
                                run_lBDDD = 1;
                            } else {
                                drop_x_low_bound = (uint16_t)(0 + 0x01FE);
                                fx_sbk(drop_x_low_bound);
                                drop_z_ptr_after_x_reset = (uint16_t)(drop_y_ptr + 4);
                                seed_carry = 1;
                                fx_stw(drop_z_ptr_after_x_reset, 0);
                                drop_ptr = (uint16_t)(drop_z_ptr_after_x_reset + 2);
                            }
                        }
                        if (run_lBDDD) {
                            run_lBDDD = 0;
                            fx_sbk(drop_x_wrapped);
                            drop_x_clamped = drop_x_wrapped;
                            haze_x8_2 = fx_ldw(FX_HAZE_X8);
                            drop_y = fx_ldw(drop_y_ptr);
                            drop_z_ptr = (uint16_t)(drop_y_ptr + 2);
                            drop_y_moved = (uint16_t)(drop_y + haze_x8_2);
                            if (!(drop_y_moved & 0x8000)) {
                                drop_y_over_max = (uint16_t)(drop_y_moved - 0x00FF);
                                drop_y_wrapped = (uint16_t)(drop_y_over_max + 0x00FF);
                                if (drop_y_over_max & 0x8000) {
                                    run_lBE0C = 1;
                                } else {
                                    drop_y_low_bound = (uint16_t)((uint16_t)~0x00FF + 1);
                                    fx_sbk(drop_y_low_bound);
                                    drop_extra_ptr_after_y_reset = (uint16_t)(drop_z_ptr + 2);
                                    seed_carry = 1;
                                    fx_stw(drop_extra_ptr_after_y_reset, 0);
                                    drop_ptr = (uint16_t)(drop_extra_ptr_after_y_reset + 2);
                                }
                            } else {
                                drop_y_biased = (uint16_t)(drop_y_moved + 0x00FF);
                                drop_y_wrapped = (uint16_t)(drop_y_biased - 0x00FF);
                                if (!(drop_y_biased & 0x8000)) {
                                    run_lBE0C = 1;
                                } else {
                                    drop_y_low_bound = (uint16_t)(0 + 0x00FE);
                                    fx_sbk(drop_y_low_bound);
                                    drop_extra_ptr_after_y_reset = (uint16_t)(drop_z_ptr + 2);
                                    seed_carry = 1;
                                    fx_stw(drop_extra_ptr_after_y_reset, 0);
                                    drop_ptr = (uint16_t)(drop_extra_ptr_after_y_reset + 2);
                                }
                            }
                            if (run_lBE0C) {
                                run_lBE0C = 0;
                                fx_sbk(drop_y_wrapped);
                                drop_y_clamped = drop_y_wrapped;
                                me_z = fx_ldw(FX_ME_Z);
                                drop_z = fx_ldw(drop_z_ptr);
                                drop_extra_ptr = (uint16_t)(drop_z_ptr + 2);
                                drop_z_moved = (uint16_t)(drop_z + me_z);
                                if (!(drop_z_moved & 0x8000)) {
                                    if ((uint16_t)(drop_z_moved - 0x01FF) & 0x8000) {
                                        fx_sbk(drop_z_moved);
                                        perspective_index = (uint16_t)(drop_z_moved + 0x012C);
                                        recip_table_addr = (uint16_t)((uint16_t)(perspective_index + perspective_index) - 0x7694);
                                        fx_r14(recip_table_addr);
                                        recip_low_byte = fx_getb();
                                        fx_r14((uint16_t)(recip_table_addr + 1));
                                        recip_scale = (uint16_t)((recip_low_byte & 0xFF) | fx_getb() << 8);
                                        /* the drop drawn from where its move began to where it ended */
                                        fx_weather_drop((int16_t)drop_x, (int16_t)drop_y, drop_z, (int16_t)drop_x_clamped, (int16_t)drop_y_clamped, drop_z_moved);
                                        screen_x_prod = (uint32_t)((int32_t)(int16_t)drop_x_clamped * (int16_t)recip_scale);
                                        view_center_x = fx_ldw(0x0034);
                                        screen_x = (uint16_t)(view_center_x + (uint16_t)((uint16_t)(screen_x_prod >> 16) << 1 | (int)(screen_x_prod >> 15 & 1)));
                                        clip_left = fx_ldw(0x0038);
                                        if ((uint16_t)(clip_left - screen_x) & 0x8000) {
                                            clip_right = fx_ldw(0x003A);
                                            if (!((uint16_t)((uint16_t)(clip_right - screen_x) - 2) & 0x8000)) {
                                                screen_y_prod = (uint32_t)((int32_t)(int16_t)drop_y_clamped * (int16_t)recip_scale);
                                                view_center_y = fx_ldw(0x0036);
                                                plot_y = (uint16_t)((uint16_t)(view_center_y + (uint16_t)((uint16_t)(screen_y_prod >> 16) << 1 | (int)(screen_y_prod >> 15 & 1))) - 0x0014);
                                                clip_top = fx_ldw(0x003C);
                                                if ((uint16_t)(clip_top - plot_y) & 0x8000) {
                                                    clip_bottom = fx_ldw(0x003E);
                                                    if (!((uint16_t)((uint16_t)(clip_bottom - plot_y) - 4) & 0x8000)) {
                                                        seed_carry = (weather >= 1);
                                                        if (weather == 1) {
                                                            prev_screen_pos = fx_ldw(drop_extra_ptr);
                                                            prev_screen_y = (uint16_t)(prev_screen_pos >> 8);
                                                            fx_stb(drop_extra_ptr, (uint8_t)screen_x);
                                                            prev_y_ptr = (uint16_t)(drop_extra_ptr + 1);
                                                            fx_stb(prev_y_ptr, (uint8_t)plot_y);
                                                            drop_ptr = (uint16_t)(prev_y_ptr + 1);
                                                            if (prev_screen_pos == 0) {
                                                            } else {
                                                                fx_push(stack, count);
                                                                fx_push(stack, loop_addr);
                                                                fx_push(stack, drop_ptr);
                                                                fx_push(stack, effect_seed);
                                                                x_step = 1;
                                                                dx = (uint16_t)((uint16_t)(prev_screen_pos & 0xFF) - screen_x);
                                                                dx_abs = dx;
                                                                if (!(dx & 0x8000)) {
                                                                } else {
                                                                    x_step = 0xFFFF;
                                                                    dx_abs = (uint16_t)((uint16_t)~dx_abs + 1);
                                                                }
                                                                y_step = 1;
                                                                dy = (uint16_t)(prev_screen_y - plot_y);
                                                                dy_abs = dy;
                                                                if (!(dy & 0x8000)) {
                                                                } else {
                                                                    y_step = 0xFFFF;
                                                                    dy_abs = (uint16_t)((uint16_t)~dy_abs + 1);
                                                                }
                                                                dx_minus_dy = (uint16_t)(dx_abs - dy_abs);
                                                                if (dx_minus_dy & 0x8000) {
                                                                    count_2 = (uint16_t)(dy_abs + 1);
                                                                    half_dy_minus_dx = (uint16_t)((uint16_t)((uint16_t)~dx_minus_dy + 1) >> 1);
                                                                    seed_carry = (half_dy_minus_dx >= dx_abs);
                                                                    line_error_steep = (uint16_t)(half_dy_minus_dx - dx_abs);
                                                                    for (;;) {
                                                                        fx_plot((uint8_t)screen_x, (uint8_t)plot_y);
                                                                        x_after_plot_steep = (uint16_t)(screen_x + 1);
                                                                        if (seed_carry) {
                                                                        } else {
                                                                            line_error_steep = (uint16_t)(line_error_steep + dy_abs);
                                                                            x_after_plot_steep = (uint16_t)(x_after_plot_steep + x_step);
                                                                        }
                                                                        screen_x = (uint16_t)(x_after_plot_steep - 1);
                                                                        plot_y = (uint16_t)(plot_y + y_step);
                                                                        count_2 = (uint16_t)(count_2 - 1);
                                                                        seed_carry = (line_error_steep >= dx_abs);
                                                                        line_error_steep = (uint16_t)(line_error_steep - dx_abs);
                                                                        if (count_2 != 0) {
                                                                            continue;
                                                                        } else {
                                                                            break;
                                                                        }
                                                                    }
                                                                } else {
                                                                    count_3 = (uint16_t)(dx_abs + 1);
                                                                    half_dx_minus_dy = (uint16_t)(dx_minus_dy >> 1);
                                                                    seed_carry = (half_dx_minus_dy >= dy_abs);
                                                                    line_error_shallow = (uint16_t)(half_dx_minus_dy - dy_abs);
                                                                    for (;;) {
                                                                        fx_plot((uint8_t)screen_x, (uint8_t)plot_y);
                                                                        x_after_plot_shallow = (uint16_t)(screen_x + 1);
                                                                        if (seed_carry) {
                                                                        } else {
                                                                            line_error_shallow = (uint16_t)(line_error_shallow + dx_abs);
                                                                            plot_y = (uint16_t)(plot_y + y_step);
                                                                        }
                                                                        screen_x = (uint16_t)((uint16_t)(x_after_plot_shallow + x_step) - 1);
                                                                        count_3 = (uint16_t)(count_3 - 1);
                                                                        seed_carry = (line_error_shallow >= dy_abs);
                                                                        line_error_shallow = (uint16_t)(line_error_shallow - dy_abs);
                                                                        if (count_3 != 0) {
                                                                            continue;
                                                                        } else {
                                                                            break;
                                                                        }
                                                                    }
                                                                }
                                                                effect_seed = fx_pull(stack);
                                                                drop_ptr = fx_pull(stack);
                                                                loop_addr = fx_pull(stack);
                                                                count = fx_pull(stack);
                                                            }
                                                        } else {
                                                            drop_ptr = (uint16_t)(drop_extra_ptr + 2);
                                                            seed_carry = (0x0100 >= drop_z_moved);
                                                            if (!((uint16_t)((uint16_t)(0 - drop_z_moved) + 0x0100) & 0x8000)) {
                                                                fx_plot((uint8_t)screen_x, (uint8_t)plot_y);
                                                                screen_x_next = (uint16_t)(screen_x + 1);
                                                                fx_plot((uint8_t)screen_x_next, (uint8_t)plot_y);
                                                                plot_y = (uint16_t)(plot_y + 1);
                                                                screen_x_back = (uint16_t)(screen_x_next - 1);
                                                                fx_plot((uint8_t)screen_x_back, (uint8_t)plot_y);
                                                                fx_plot((uint8_t)(uint16_t)(screen_x_back + 1), (uint8_t)plot_y);
                                                            } else {
                                                                fx_plot((uint8_t)screen_x, (uint8_t)plot_y);
                                                            }
                                                        }
                                                    } else {
                                                        drop_life_ptr = (uint16_t)(drop_extra_ptr - 8);
                                                        seed_carry = ((uint32_t)drop_life_ptr + 0 > 0xFFFF);
                                                        fx_stb(drop_life_ptr, (uint8_t)1);
                                                        drop_ptr = (uint16_t)(drop_extra_ptr + 2);
                                                    }
                                                } else {
                                                    seed_carry = 1;
                                                    fx_stw(drop_extra_ptr, 0);
                                                    drop_ptr = (uint16_t)(drop_extra_ptr + 2);
                                                }
                                            } else {
                                                seed_carry = 1;
                                                fx_stw(drop_extra_ptr, 0);
                                                drop_ptr = (uint16_t)(drop_extra_ptr + 2);
                                            }
                                        } else {
                                            seed_carry = 1;
                                            fx_stw(drop_extra_ptr, 0);
                                            drop_ptr = (uint16_t)(drop_extra_ptr + 2);
                                        }
                                    } else {
                                        drop_z_bound = 0;
                                        fx_sbk(drop_z_bound);
                                        seed_carry = 1;
                                        fx_stw(drop_extra_ptr, 0);
                                        drop_ptr = (uint16_t)(drop_extra_ptr + 2);
                                    }
                                } else {
                                    drop_z_bound = 0x01FF;
                                    fx_sbk(drop_z_bound);
                                    seed_carry = 1;
                                    fx_stw(drop_extra_ptr, 0);
                                    drop_ptr = (uint16_t)(drop_extra_ptr + 2);
                                }
                            }
                        }
                    } else {
                        fx_stb(drop_ptr, (uint8_t)(uint16_t)(drop_life_less_one + 2));
                        seed_carry = ((uint32_t)drop_ptr + 0x000A > 0xFFFF);
                        drop_ptr = (uint16_t)(drop_ptr + 0x000A);
                    }
                }
                count = (uint16_t)(count - 1);
                if (count != 0) {
                    continue;
                } else {
                    break;
                }
            }
        } else {
        }
    } else {
    }
    fx_weather_end();
    fx_stw(0x01D0, effect_seed);
    fx_unspill(stack);
    return plot_y;
}


/*
 * @routine 01:BF2F FxWeatherUnused
 * WHAT IT DOES: Code no jump or branch reaches: a branch back into
 * FxWeatherDraw's drop loop (01:BF2A); nothing in bank 01 jumps or
 * branches here, so a direct call is its only way in. The code after
 * it is three more such entries: FxWeatherUnusedLoop,
 * FxWeatherUnusedClear and FxWeatherUnusedFull.
 * HOW: BRA with a NOP in its delay slot; then as FxWeatherDraw.
 * Entry: as FxWeatherDraw's drop loop: R12 = the drops left, R13 = the
 * loop, R10 = stack. Exit: as FxWeatherDraw.
 */
FxWeatherUnusedResult FxWeatherUnused(uint16_t seed, uint16_t drop_at, uint16_t *stack, uint16_t weather_kind, uint16_t count, int past_drop_end)
{
    int run_lBDDD = 0;
    int run_lBE0C = 0;
    uint16_t dx;
    uint16_t dy;
    uint16_t me_z;
    uint16_t drop_x;
    uint16_t drop_y;
    uint16_t drop_z;
    uint16_t dx_abs;
    uint16_t dy_abs;
    uint16_t x_step;
    uint16_t y_step;
    uint16_t count_2;
    uint16_t count_3;
    uint16_t haze_x8;
    uint16_t clip_top;
    uint16_t new_life;
    uint16_t screen_x;
    uint16_t clip_left;
    uint16_t drop_life;
    uint16_t clip_right;
    uint16_t drop_x_ptr;
    uint16_t drop_y_ptr;
    uint16_t drop_z_ptr;
    uint16_t new_drop_z;
    uint16_t prev_y_ptr;
    uint16_t saved_loop;
    uint16_t seed_sum_2;
    uint16_t seed_sum_3;
    uint16_t clip_bottom;
    uint16_t drop_colour;
    uint16_t dx_minus_dy;
    uint16_t recip_scale;
    uint16_t seed_next_1;
    uint16_t drop_x_moved;
    uint16_t drop_y_moved;
    uint16_t drop_z_bound;
    uint16_t drop_z_moved;
    uint16_t screen_y_top;
    int seed_carry_2;
    uint16_t drift_x_delta;
    uint16_t drop_life_ptr;
    uint16_t drop_life_raw;
    uint16_t drop_x_biased;
    uint16_t drop_y_biased;
    uint16_t prev_screen_y;
    uint16_t screen_x_back;
    uint16_t screen_x_next;
    uint32_t screen_x_prod;
    uint32_t screen_y_prod;
    uint16_t view_center_x;
    uint16_t view_center_y;
    uint16_t drop_extra_ptr;
    uint16_t drop_x_clamped;
    uint16_t drop_x_ptr_new;
    uint16_t drop_x_wrapped;
    uint16_t drop_y_clamped;
    uint16_t drop_y_ptr_new;
    uint16_t drop_y_wrapped;
    uint16_t drop_z_ptr_new;
    uint16_t recip_low_byte;
    uint16_t screen_y_below;
    uint16_t seed_rotated_2;
    uint16_t seed_rotated_3;
    uint16_t weather_colour;
    uint16_t drop_colour_ptr;
    uint16_t drop_x_over_max;
    uint16_t drop_y_over_max;
    uint16_t prev_screen_pos;
    uint16_t drop_x_low_bound;
    uint16_t drop_y_low_bound;
    uint16_t half_dx_minus_dy;
    uint16_t half_dy_minus_dx;
    uint16_t line_error_steep;
    uint16_t recip_table_addr;
    uint16_t perspective_index;
    uint16_t view_center_y_new;
    uint16_t drop_extra_ptr_new;
    uint16_t drop_life_less_one;
    uint16_t line_error_shallow;
    uint16_t x_after_plot_steep;
    uint16_t drop_colour_ptr_move;
    uint16_t x_after_plot_shallow;
    uint16_t drop_z_ptr_after_x_reset;
    uint16_t drop_extra_ptr_after_y_reset;

    for (;;) {
        count = (uint16_t)(count - 1);
        if (count != 0) {
            drop_life_raw = fx_ldb(drop_at);
            drop_life = (uint16_t)(int16_t)(int8_t)drop_life_raw;
            drop_life_less_one = (uint16_t)(drop_life - 1);
            if (drop_life == 0) {
                new_life = fx_ldw(0x26FC);
                fx_stb(drop_at, (uint8_t)new_life);
                drop_colour_ptr = (uint16_t)(drop_at + 1);
                seed_rotated_2 = (uint16_t)((uint16_t)(seed << 8 | seed >> 8) >> 1 | past_drop_end << 15);
                seed_sum_2 = (uint16_t)(seed_rotated_2 + seed);
                seed_carry_2 = ((uint32_t)seed_rotated_2 + seed > 0xFFFF);
                seed_next_1 = (uint16_t)(seed_sum_2 + seed + seed_carry_2);
                weather_colour = fx_ldw(FX_WEATHER_COLOUR);
                fx_stb(drop_colour_ptr, (uint8_t)weather_colour);
                drop_x_ptr_new = (uint16_t)(drop_colour_ptr + 1);
                new_drop_z = (weather_colour & 0x01FF);
                seed_rotated_3 = (uint16_t)((uint16_t)(seed_next_1 << 8 | seed_next_1 >> 8) >> 1 | ((uint32_t)seed_sum_2 + seed + seed_carry_2 > 0xFFFF) << 15);
                seed_sum_3 = (uint16_t)(seed_rotated_3 + seed_next_1);
                seed = (uint16_t)(seed_sum_3 + seed_next_1 + ((uint32_t)seed_rotated_3 + seed_next_1 > 0xFFFF));
                fx_stw(drop_x_ptr_new, (uint16_t)((seed_sum_3 & 0x03FF) - 0x01FF));
                drop_y_ptr_new = (uint16_t)(drop_x_ptr_new + 2);
                view_center_y_new = fx_ldw(0x0036);
                fx_stw(drop_y_ptr_new, (uint16_t)((uint16_t)~(uint16_t)(fx_div2(new_drop_z) + view_center_y_new) + 1));
                drop_z_ptr_new = (uint16_t)(drop_y_ptr_new + 2);
                fx_stw(drop_z_ptr_new, new_drop_z);
                drop_extra_ptr_new = (uint16_t)(drop_z_ptr_new + 2);
                past_drop_end = 1;
                fx_stw(drop_extra_ptr_new, 0);
                drop_at = (uint16_t)(drop_extra_ptr_new + 2);
                continue;
            } else {
                if (!(drop_life_less_one & 0x8000)) {
                    fx_stb(drop_at, (uint8_t)drop_life_less_one);
                    drop_colour_ptr_move = (uint16_t)(drop_at + 1);
                    drop_colour = fx_ldb(drop_colour_ptr_move);
                    drop_x_ptr = (uint16_t)(drop_colour_ptr_move + 1);
                    fx_color((uint8_t)drop_colour);
                    drift_x_delta = fx_ldw(0x26F6);
                    drop_x = fx_ldw(drop_x_ptr);
                    drop_y_ptr = (uint16_t)(drop_x_ptr + 2);
                    drop_x_moved = (uint16_t)(drop_x + drift_x_delta);
                    if (!(drop_x_moved & 0x8000)) {
                        drop_x_over_max = (uint16_t)(drop_x_moved - 0x01FF);
                        drop_x_wrapped = (uint16_t)(drop_x_over_max + 0x01FF);
                        if (drop_x_over_max & 0x8000) {
                            run_lBDDD = 1;
                        } else {
                            drop_x_low_bound = (uint16_t)((uint16_t)~0x01FF + 1);
                            fx_sbk(drop_x_low_bound);
                            drop_z_ptr_after_x_reset = (uint16_t)(drop_y_ptr + 4);
                            past_drop_end = 1;
                            fx_stw(drop_z_ptr_after_x_reset, 0);
                            drop_at = (uint16_t)(drop_z_ptr_after_x_reset + 2);
                            continue;
                        }
                    } else {
                        drop_x_biased = (uint16_t)(drop_x_moved + 0x01FF);
                        drop_x_wrapped = (uint16_t)(drop_x_biased - 0x01FF);
                        if (!(drop_x_biased & 0x8000)) {
                            run_lBDDD = 1;
                        } else {
                            drop_x_low_bound = (uint16_t)(0 + 0x01FE);
                            fx_sbk(drop_x_low_bound);
                            drop_z_ptr_after_x_reset = (uint16_t)(drop_y_ptr + 4);
                            past_drop_end = 1;
                            fx_stw(drop_z_ptr_after_x_reset, 0);
                            drop_at = (uint16_t)(drop_z_ptr_after_x_reset + 2);
                            continue;
                        }
                    }
                    fx_sbk(drop_x_wrapped);
                    drop_x_clamped = drop_x_wrapped;
                    haze_x8 = fx_ldw(FX_HAZE_X8);
                    drop_y = fx_ldw(drop_y_ptr);
                    drop_z_ptr = (uint16_t)(drop_y_ptr + 2);
                    drop_y_moved = (uint16_t)(drop_y + haze_x8);
                    if (!(drop_y_moved & 0x8000)) {
                        drop_y_over_max = (uint16_t)(drop_y_moved - 0x00FF);
                        drop_y_wrapped = (uint16_t)(drop_y_over_max + 0x00FF);
                        if (drop_y_over_max & 0x8000) {
                            run_lBE0C = 1;
                        } else {
                            drop_y_low_bound = (uint16_t)((uint16_t)~0x00FF + 1);
                            fx_sbk(drop_y_low_bound);
                            drop_extra_ptr_after_y_reset = (uint16_t)(drop_z_ptr + 2);
                            past_drop_end = 1;
                            fx_stw(drop_extra_ptr_after_y_reset, 0);
                            drop_at = (uint16_t)(drop_extra_ptr_after_y_reset + 2);
                            continue;
                        }
                    } else {
                        drop_y_biased = (uint16_t)(drop_y_moved + 0x00FF);
                        drop_y_wrapped = (uint16_t)(drop_y_biased - 0x00FF);
                        if (!(drop_y_biased & 0x8000)) {
                            run_lBE0C = 1;
                        } else {
                            drop_y_low_bound = (uint16_t)(0 + 0x00FE);
                            fx_sbk(drop_y_low_bound);
                            drop_extra_ptr_after_y_reset = (uint16_t)(drop_z_ptr + 2);
                            past_drop_end = 1;
                            fx_stw(drop_extra_ptr_after_y_reset, 0);
                            drop_at = (uint16_t)(drop_extra_ptr_after_y_reset + 2);
                            continue;
                        }
                    }
                    fx_sbk(drop_y_wrapped);
                    drop_y_clamped = drop_y_wrapped;
                    me_z = fx_ldw(FX_ME_Z);
                    drop_z = fx_ldw(drop_z_ptr);
                    drop_extra_ptr = (uint16_t)(drop_z_ptr + 2);
                    drop_z_moved = (uint16_t)(drop_z + me_z);
                    if (!(drop_z_moved & 0x8000)) {
                        if ((uint16_t)(drop_z_moved - 0x01FF) & 0x8000) {
                            fx_sbk(drop_z_moved);
                            perspective_index = (uint16_t)(drop_z_moved + 0x012C);
                            recip_table_addr = (uint16_t)((uint16_t)(perspective_index + perspective_index) - 0x7694);
                            fx_r14(recip_table_addr);
                            recip_low_byte = fx_getb();
                            fx_r14((uint16_t)(recip_table_addr + 1));
                            recip_scale = (uint16_t)((recip_low_byte & 0xFF) | fx_getb() << 8);
                            screen_x_prod = (uint32_t)((int32_t)(int16_t)drop_x_clamped * (int16_t)recip_scale);
                            view_center_x = fx_ldw(0x0034);
                            screen_x = (uint16_t)(view_center_x + (uint16_t)((uint16_t)(screen_x_prod >> 16) << 1 | (int)(screen_x_prod >> 15 & 1)));
                            clip_left = fx_ldw(0x0038);
                            if ((uint16_t)(clip_left - screen_x) & 0x8000) {
                                clip_right = fx_ldw(0x003A);
                                if (!((uint16_t)((uint16_t)(clip_right - screen_x) - 2) & 0x8000)) {
                                    screen_y_prod = (uint32_t)((int32_t)(int16_t)drop_y_clamped * (int16_t)recip_scale);
                                    view_center_y = fx_ldw(0x0036);
                                    screen_y_top = (uint16_t)((uint16_t)(view_center_y + (uint16_t)((uint16_t)(screen_y_prod >> 16) << 1 | (int)(screen_y_prod >> 15 & 1))) - 0x0014);
                                    clip_top = fx_ldw(0x003C);
                                    if ((uint16_t)(clip_top - screen_y_top) & 0x8000) {
                                        clip_bottom = fx_ldw(0x003E);
                                        if (!((uint16_t)((uint16_t)(clip_bottom - screen_y_top) - 4) & 0x8000)) {
                                            past_drop_end = (weather_kind >= 1);
                                            if (weather_kind == 1) {
                                                prev_screen_pos = fx_ldw(drop_extra_ptr);
                                                prev_screen_y = (uint16_t)(prev_screen_pos >> 8);
                                                fx_stb(drop_extra_ptr, (uint8_t)screen_x);
                                                prev_y_ptr = (uint16_t)(drop_extra_ptr + 1);
                                                fx_stb(prev_y_ptr, (uint8_t)screen_y_top);
                                                drop_at = (uint16_t)(prev_y_ptr + 1);
                                                if (prev_screen_pos == 0) {
                                                    continue;
                                                } else {
                                                    fx_push(stack, count);
                                                    fx_push(stack, saved_loop);
                                                    fx_push(stack, drop_at);
                                                    fx_push(stack, seed);
                                                    x_step = 1;
                                                    dx = (uint16_t)((uint16_t)(prev_screen_pos & 0xFF) - screen_x);
                                                    dx_abs = dx;
                                                    if (!(dx & 0x8000)) {
                                                    } else {
                                                        x_step = 0xFFFF;
                                                        dx_abs = (uint16_t)((uint16_t)~dx_abs + 1);
                                                    }
                                                    y_step = 1;
                                                    dy = (uint16_t)(prev_screen_y - screen_y_top);
                                                    dy_abs = dy;
                                                    if (!(dy & 0x8000)) {
                                                    } else {
                                                        y_step = 0xFFFF;
                                                        dy_abs = (uint16_t)((uint16_t)~dy_abs + 1);
                                                    }
                                                    dx_minus_dy = (uint16_t)(dx_abs - dy_abs);
                                                    if (dx_minus_dy & 0x8000) {
                                                        count_2 = (uint16_t)(dy_abs + 1);
                                                        half_dy_minus_dx = (uint16_t)((uint16_t)((uint16_t)~dx_minus_dy + 1) >> 1);
                                                        past_drop_end = (half_dy_minus_dx >= dx_abs);
                                                        line_error_steep = (uint16_t)(half_dy_minus_dx - dx_abs);
                                                        for (;;) {
                                                            fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                            x_after_plot_steep = (uint16_t)(screen_x + 1);
                                                            if (past_drop_end) {
                                                            } else {
                                                                line_error_steep = (uint16_t)(line_error_steep + dy_abs);
                                                                x_after_plot_steep = (uint16_t)(x_after_plot_steep + x_step);
                                                            }
                                                            screen_x = (uint16_t)(x_after_plot_steep - 1);
                                                            screen_y_top = (uint16_t)(screen_y_top + y_step);
                                                            count_2 = (uint16_t)(count_2 - 1);
                                                            past_drop_end = (line_error_steep >= dx_abs);
                                                            line_error_steep = (uint16_t)(line_error_steep - dx_abs);
                                                            if (count_2 != 0) {
                                                                continue;
                                                            } else {
                                                                break;
                                                            }
                                                        }
                                                    } else {
                                                        count_3 = (uint16_t)(dx_abs + 1);
                                                        half_dx_minus_dy = (uint16_t)(dx_minus_dy >> 1);
                                                        past_drop_end = (half_dx_minus_dy >= dy_abs);
                                                        line_error_shallow = (uint16_t)(half_dx_minus_dy - dy_abs);
                                                        for (;;) {
                                                            fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                            x_after_plot_shallow = (uint16_t)(screen_x + 1);
                                                            if (past_drop_end) {
                                                            } else {
                                                                line_error_shallow = (uint16_t)(line_error_shallow + dx_abs);
                                                                screen_y_top = (uint16_t)(screen_y_top + y_step);
                                                            }
                                                            screen_x = (uint16_t)((uint16_t)(x_after_plot_shallow + x_step) - 1);
                                                            count_3 = (uint16_t)(count_3 - 1);
                                                            past_drop_end = (line_error_shallow >= dy_abs);
                                                            line_error_shallow = (uint16_t)(line_error_shallow - dy_abs);
                                                            if (count_3 != 0) {
                                                                continue;
                                                            } else {
                                                                break;
                                                            }
                                                        }
                                                    }
                                                    seed = fx_pull(stack);
                                                    drop_at = fx_pull(stack);
                                                    saved_loop = fx_pull(stack);
                                                    count = fx_pull(stack);
                                                    continue;
                                                }
                                            } else {
                                                drop_at = (uint16_t)(drop_extra_ptr + 2);
                                                past_drop_end = (0x0100 >= drop_z_moved);
                                                if (!((uint16_t)((uint16_t)(0 - drop_z_moved) + 0x0100) & 0x8000)) {
                                                    fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                    screen_x_next = (uint16_t)(screen_x + 1);
                                                    fx_plot((uint8_t)screen_x_next, (uint8_t)screen_y_top);
                                                    screen_y_below = (uint16_t)(screen_y_top + 1);
                                                    screen_x_back = (uint16_t)(screen_x_next - 1);
                                                    fx_plot((uint8_t)screen_x_back, (uint8_t)screen_y_below);
                                                    fx_plot((uint8_t)(uint16_t)(screen_x_back + 1), (uint8_t)screen_y_below);
                                                    continue;
                                                } else {
                                                    fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                    continue;
                                                }
                                            }
                                        } else {
                                            drop_life_ptr = (uint16_t)(drop_extra_ptr - 8);
                                            past_drop_end = ((uint32_t)drop_life_ptr + 0 > 0xFFFF);
                                            fx_stb(drop_life_ptr, (uint8_t)1);
                                            drop_at = (uint16_t)(drop_extra_ptr + 2);
                                            continue;
                                        }
                                    } else {
                                        past_drop_end = 1;
                                        fx_stw(drop_extra_ptr, 0);
                                        drop_at = (uint16_t)(drop_extra_ptr + 2);
                                        continue;
                                    }
                                } else {
                                    past_drop_end = 1;
                                    fx_stw(drop_extra_ptr, 0);
                                    drop_at = (uint16_t)(drop_extra_ptr + 2);
                                    continue;
                                }
                            } else {
                                past_drop_end = 1;
                                fx_stw(drop_extra_ptr, 0);
                                drop_at = (uint16_t)(drop_extra_ptr + 2);
                                continue;
                            }
                        } else {
                            drop_z_bound = 0;
                            fx_sbk(drop_z_bound);
                            past_drop_end = 1;
                            fx_stw(drop_extra_ptr, 0);
                            drop_at = (uint16_t)(drop_extra_ptr + 2);
                            continue;
                        }
                    } else {
                        drop_z_bound = 0x01FF;
                        fx_sbk(drop_z_bound);
                        past_drop_end = 1;
                        fx_stw(drop_extra_ptr, 0);
                        drop_at = (uint16_t)(drop_extra_ptr + 2);
                        continue;
                    }
                } else {
                    fx_stb(drop_at, (uint8_t)(uint16_t)(drop_life_less_one + 2));
                    past_drop_end = ((uint32_t)drop_at + 0x000A > 0xFFFF);
                    drop_at = (uint16_t)(drop_at + 0x000A);
                    continue;
                }
            }
        } else {
            break;
        }
    }
    fx_stw(0x01D0, seed);
    fx_unspill(stack);
    return (FxWeatherUnusedResult){1};
}


/*
 * @routine 01:BF32 FxWeatherUnusedLoop
 * WHAT IT DOES: The second of FxWeatherUnused's branches back into
 * FxWeatherDraw's drop loop (01:BF2A), which nothing reaches; a direct
 * call is its only way in.
 * HOW: BRA with a NOP in its delay slot; then as FxWeatherDraw.
 * Entry: as FxWeatherDraw's drop loop: R12 = the drops left, R13 = the
 * loop, R10 = stack. Exit: as FxWeatherDraw.
 */
FxWeatherUnusedLoopResult FxWeatherUnusedLoop(uint16_t seed, uint16_t drop_at, uint16_t *stack, uint16_t weather_kind, uint16_t count, int past_drop_end)
{
    int run_lBDDD = 0;
    int run_lBE0C = 0;
    uint16_t dx;
    uint16_t dy;
    uint16_t me_z;
    uint16_t drop_x;
    uint16_t drop_y;
    uint16_t drop_z;
    uint16_t dx_abs;
    uint16_t dy_abs;
    uint16_t x_step;
    uint16_t y_step;
    uint16_t count_2;
    uint16_t count_3;
    uint16_t haze_x8;
    uint16_t clip_top;
    uint16_t new_life;
    uint16_t screen_x;
    uint16_t clip_left;
    uint16_t drop_life;
    uint16_t clip_right;
    uint16_t drop_x_ptr;
    uint16_t drop_y_ptr;
    uint16_t drop_z_ptr;
    uint16_t new_drop_z;
    uint16_t prev_y_ptr;
    uint16_t saved_loop;
    uint16_t seed_sum_2;
    uint16_t seed_sum_3;
    uint16_t clip_bottom;
    uint16_t drop_colour;
    uint16_t dx_minus_dy;
    uint16_t recip_scale;
    uint16_t seed_next_1;
    uint16_t drop_x_moved;
    uint16_t drop_y_moved;
    uint16_t drop_z_bound;
    uint16_t drop_z_moved;
    uint16_t screen_y_top;
    int seed_carry_2;
    uint16_t drift_x_delta;
    uint16_t drop_life_ptr;
    uint16_t drop_life_raw;
    uint16_t drop_x_biased;
    uint16_t drop_y_biased;
    uint16_t prev_screen_y;
    uint16_t screen_x_back;
    uint16_t screen_x_next;
    uint32_t screen_x_prod;
    uint32_t screen_y_prod;
    uint16_t view_center_x;
    uint16_t view_center_y;
    uint16_t drop_extra_ptr;
    uint16_t drop_x_clamped;
    uint16_t drop_x_ptr_new;
    uint16_t drop_x_wrapped;
    uint16_t drop_y_clamped;
    uint16_t drop_y_ptr_new;
    uint16_t drop_y_wrapped;
    uint16_t drop_z_ptr_new;
    uint16_t recip_low_byte;
    uint16_t screen_y_below;
    uint16_t seed_rotated_2;
    uint16_t seed_rotated_3;
    uint16_t weather_colour;
    uint16_t drop_colour_ptr;
    uint16_t drop_x_over_max;
    uint16_t drop_y_over_max;
    uint16_t prev_screen_pos;
    uint16_t drop_x_low_bound;
    uint16_t drop_y_low_bound;
    uint16_t half_dx_minus_dy;
    uint16_t half_dy_minus_dx;
    uint16_t line_error_steep;
    uint16_t recip_table_addr;
    uint16_t perspective_index;
    uint16_t view_center_y_new;
    uint16_t drop_extra_ptr_new;
    uint16_t drop_life_less_one;
    uint16_t line_error_shallow;
    uint16_t x_after_plot_steep;
    uint16_t drop_colour_ptr_move;
    uint16_t x_after_plot_shallow;
    uint16_t drop_z_ptr_after_x_reset;
    uint16_t drop_extra_ptr_after_y_reset;

    for (;;) {
        count = (uint16_t)(count - 1);
        if (count != 0) {
            drop_life_raw = fx_ldb(drop_at);
            drop_life = (uint16_t)(int16_t)(int8_t)drop_life_raw;
            drop_life_less_one = (uint16_t)(drop_life - 1);
            if (drop_life == 0) {
                new_life = fx_ldw(0x26FC);
                fx_stb(drop_at, (uint8_t)new_life);
                drop_colour_ptr = (uint16_t)(drop_at + 1);
                seed_rotated_2 = (uint16_t)((uint16_t)(seed << 8 | seed >> 8) >> 1 | past_drop_end << 15);
                seed_sum_2 = (uint16_t)(seed_rotated_2 + seed);
                seed_carry_2 = ((uint32_t)seed_rotated_2 + seed > 0xFFFF);
                seed_next_1 = (uint16_t)(seed_sum_2 + seed + seed_carry_2);
                weather_colour = fx_ldw(FX_WEATHER_COLOUR);
                fx_stb(drop_colour_ptr, (uint8_t)weather_colour);
                drop_x_ptr_new = (uint16_t)(drop_colour_ptr + 1);
                new_drop_z = (weather_colour & 0x01FF);
                seed_rotated_3 = (uint16_t)((uint16_t)(seed_next_1 << 8 | seed_next_1 >> 8) >> 1 | ((uint32_t)seed_sum_2 + seed + seed_carry_2 > 0xFFFF) << 15);
                seed_sum_3 = (uint16_t)(seed_rotated_3 + seed_next_1);
                seed = (uint16_t)(seed_sum_3 + seed_next_1 + ((uint32_t)seed_rotated_3 + seed_next_1 > 0xFFFF));
                fx_stw(drop_x_ptr_new, (uint16_t)((seed_sum_3 & 0x03FF) - 0x01FF));
                drop_y_ptr_new = (uint16_t)(drop_x_ptr_new + 2);
                view_center_y_new = fx_ldw(0x0036);
                fx_stw(drop_y_ptr_new, (uint16_t)((uint16_t)~(uint16_t)(fx_div2(new_drop_z) + view_center_y_new) + 1));
                drop_z_ptr_new = (uint16_t)(drop_y_ptr_new + 2);
                fx_stw(drop_z_ptr_new, new_drop_z);
                drop_extra_ptr_new = (uint16_t)(drop_z_ptr_new + 2);
                past_drop_end = 1;
                fx_stw(drop_extra_ptr_new, 0);
                drop_at = (uint16_t)(drop_extra_ptr_new + 2);
                continue;
            } else {
                if (!(drop_life_less_one & 0x8000)) {
                    fx_stb(drop_at, (uint8_t)drop_life_less_one);
                    drop_colour_ptr_move = (uint16_t)(drop_at + 1);
                    drop_colour = fx_ldb(drop_colour_ptr_move);
                    drop_x_ptr = (uint16_t)(drop_colour_ptr_move + 1);
                    fx_color((uint8_t)drop_colour);
                    drift_x_delta = fx_ldw(0x26F6);
                    drop_x = fx_ldw(drop_x_ptr);
                    drop_y_ptr = (uint16_t)(drop_x_ptr + 2);
                    drop_x_moved = (uint16_t)(drop_x + drift_x_delta);
                    if (!(drop_x_moved & 0x8000)) {
                        drop_x_over_max = (uint16_t)(drop_x_moved - 0x01FF);
                        drop_x_wrapped = (uint16_t)(drop_x_over_max + 0x01FF);
                        if (drop_x_over_max & 0x8000) {
                            run_lBDDD = 1;
                        } else {
                            drop_x_low_bound = (uint16_t)((uint16_t)~0x01FF + 1);
                            fx_sbk(drop_x_low_bound);
                            drop_z_ptr_after_x_reset = (uint16_t)(drop_y_ptr + 4);
                            past_drop_end = 1;
                            fx_stw(drop_z_ptr_after_x_reset, 0);
                            drop_at = (uint16_t)(drop_z_ptr_after_x_reset + 2);
                            continue;
                        }
                    } else {
                        drop_x_biased = (uint16_t)(drop_x_moved + 0x01FF);
                        drop_x_wrapped = (uint16_t)(drop_x_biased - 0x01FF);
                        if (!(drop_x_biased & 0x8000)) {
                            run_lBDDD = 1;
                        } else {
                            drop_x_low_bound = (uint16_t)(0 + 0x01FE);
                            fx_sbk(drop_x_low_bound);
                            drop_z_ptr_after_x_reset = (uint16_t)(drop_y_ptr + 4);
                            past_drop_end = 1;
                            fx_stw(drop_z_ptr_after_x_reset, 0);
                            drop_at = (uint16_t)(drop_z_ptr_after_x_reset + 2);
                            continue;
                        }
                    }
                    fx_sbk(drop_x_wrapped);
                    drop_x_clamped = drop_x_wrapped;
                    haze_x8 = fx_ldw(FX_HAZE_X8);
                    drop_y = fx_ldw(drop_y_ptr);
                    drop_z_ptr = (uint16_t)(drop_y_ptr + 2);
                    drop_y_moved = (uint16_t)(drop_y + haze_x8);
                    if (!(drop_y_moved & 0x8000)) {
                        drop_y_over_max = (uint16_t)(drop_y_moved - 0x00FF);
                        drop_y_wrapped = (uint16_t)(drop_y_over_max + 0x00FF);
                        if (drop_y_over_max & 0x8000) {
                            run_lBE0C = 1;
                        } else {
                            drop_y_low_bound = (uint16_t)((uint16_t)~0x00FF + 1);
                            fx_sbk(drop_y_low_bound);
                            drop_extra_ptr_after_y_reset = (uint16_t)(drop_z_ptr + 2);
                            past_drop_end = 1;
                            fx_stw(drop_extra_ptr_after_y_reset, 0);
                            drop_at = (uint16_t)(drop_extra_ptr_after_y_reset + 2);
                            continue;
                        }
                    } else {
                        drop_y_biased = (uint16_t)(drop_y_moved + 0x00FF);
                        drop_y_wrapped = (uint16_t)(drop_y_biased - 0x00FF);
                        if (!(drop_y_biased & 0x8000)) {
                            run_lBE0C = 1;
                        } else {
                            drop_y_low_bound = (uint16_t)(0 + 0x00FE);
                            fx_sbk(drop_y_low_bound);
                            drop_extra_ptr_after_y_reset = (uint16_t)(drop_z_ptr + 2);
                            past_drop_end = 1;
                            fx_stw(drop_extra_ptr_after_y_reset, 0);
                            drop_at = (uint16_t)(drop_extra_ptr_after_y_reset + 2);
                            continue;
                        }
                    }
                    fx_sbk(drop_y_wrapped);
                    drop_y_clamped = drop_y_wrapped;
                    me_z = fx_ldw(FX_ME_Z);
                    drop_z = fx_ldw(drop_z_ptr);
                    drop_extra_ptr = (uint16_t)(drop_z_ptr + 2);
                    drop_z_moved = (uint16_t)(drop_z + me_z);
                    if (!(drop_z_moved & 0x8000)) {
                        if ((uint16_t)(drop_z_moved - 0x01FF) & 0x8000) {
                            fx_sbk(drop_z_moved);
                            perspective_index = (uint16_t)(drop_z_moved + 0x012C);
                            recip_table_addr = (uint16_t)((uint16_t)(perspective_index + perspective_index) - 0x7694);
                            fx_r14(recip_table_addr);
                            recip_low_byte = fx_getb();
                            fx_r14((uint16_t)(recip_table_addr + 1));
                            recip_scale = (uint16_t)((recip_low_byte & 0xFF) | fx_getb() << 8);
                            screen_x_prod = (uint32_t)((int32_t)(int16_t)drop_x_clamped * (int16_t)recip_scale);
                            view_center_x = fx_ldw(0x0034);
                            screen_x = (uint16_t)(view_center_x + (uint16_t)((uint16_t)(screen_x_prod >> 16) << 1 | (int)(screen_x_prod >> 15 & 1)));
                            clip_left = fx_ldw(0x0038);
                            if ((uint16_t)(clip_left - screen_x) & 0x8000) {
                                clip_right = fx_ldw(0x003A);
                                if (!((uint16_t)((uint16_t)(clip_right - screen_x) - 2) & 0x8000)) {
                                    screen_y_prod = (uint32_t)((int32_t)(int16_t)drop_y_clamped * (int16_t)recip_scale);
                                    view_center_y = fx_ldw(0x0036);
                                    screen_y_top = (uint16_t)((uint16_t)(view_center_y + (uint16_t)((uint16_t)(screen_y_prod >> 16) << 1 | (int)(screen_y_prod >> 15 & 1))) - 0x0014);
                                    clip_top = fx_ldw(0x003C);
                                    if ((uint16_t)(clip_top - screen_y_top) & 0x8000) {
                                        clip_bottom = fx_ldw(0x003E);
                                        if (!((uint16_t)((uint16_t)(clip_bottom - screen_y_top) - 4) & 0x8000)) {
                                            past_drop_end = (weather_kind >= 1);
                                            if (weather_kind == 1) {
                                                prev_screen_pos = fx_ldw(drop_extra_ptr);
                                                prev_screen_y = (uint16_t)(prev_screen_pos >> 8);
                                                fx_stb(drop_extra_ptr, (uint8_t)screen_x);
                                                prev_y_ptr = (uint16_t)(drop_extra_ptr + 1);
                                                fx_stb(prev_y_ptr, (uint8_t)screen_y_top);
                                                drop_at = (uint16_t)(prev_y_ptr + 1);
                                                if (prev_screen_pos == 0) {
                                                    continue;
                                                } else {
                                                    fx_push(stack, count);
                                                    fx_push(stack, saved_loop);
                                                    fx_push(stack, drop_at);
                                                    fx_push(stack, seed);
                                                    x_step = 1;
                                                    dx = (uint16_t)((uint16_t)(prev_screen_pos & 0xFF) - screen_x);
                                                    dx_abs = dx;
                                                    if (!(dx & 0x8000)) {
                                                    } else {
                                                        x_step = 0xFFFF;
                                                        dx_abs = (uint16_t)((uint16_t)~dx_abs + 1);
                                                    }
                                                    y_step = 1;
                                                    dy = (uint16_t)(prev_screen_y - screen_y_top);
                                                    dy_abs = dy;
                                                    if (!(dy & 0x8000)) {
                                                    } else {
                                                        y_step = 0xFFFF;
                                                        dy_abs = (uint16_t)((uint16_t)~dy_abs + 1);
                                                    }
                                                    dx_minus_dy = (uint16_t)(dx_abs - dy_abs);
                                                    if (dx_minus_dy & 0x8000) {
                                                        count_2 = (uint16_t)(dy_abs + 1);
                                                        half_dy_minus_dx = (uint16_t)((uint16_t)((uint16_t)~dx_minus_dy + 1) >> 1);
                                                        past_drop_end = (half_dy_minus_dx >= dx_abs);
                                                        line_error_steep = (uint16_t)(half_dy_minus_dx - dx_abs);
                                                        for (;;) {
                                                            fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                            x_after_plot_steep = (uint16_t)(screen_x + 1);
                                                            if (past_drop_end) {
                                                            } else {
                                                                line_error_steep = (uint16_t)(line_error_steep + dy_abs);
                                                                x_after_plot_steep = (uint16_t)(x_after_plot_steep + x_step);
                                                            }
                                                            screen_x = (uint16_t)(x_after_plot_steep - 1);
                                                            screen_y_top = (uint16_t)(screen_y_top + y_step);
                                                            count_2 = (uint16_t)(count_2 - 1);
                                                            past_drop_end = (line_error_steep >= dx_abs);
                                                            line_error_steep = (uint16_t)(line_error_steep - dx_abs);
                                                            if (count_2 != 0) {
                                                                continue;
                                                            } else {
                                                                break;
                                                            }
                                                        }
                                                    } else {
                                                        count_3 = (uint16_t)(dx_abs + 1);
                                                        half_dx_minus_dy = (uint16_t)(dx_minus_dy >> 1);
                                                        past_drop_end = (half_dx_minus_dy >= dy_abs);
                                                        line_error_shallow = (uint16_t)(half_dx_minus_dy - dy_abs);
                                                        for (;;) {
                                                            fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                            x_after_plot_shallow = (uint16_t)(screen_x + 1);
                                                            if (past_drop_end) {
                                                            } else {
                                                                line_error_shallow = (uint16_t)(line_error_shallow + dx_abs);
                                                                screen_y_top = (uint16_t)(screen_y_top + y_step);
                                                            }
                                                            screen_x = (uint16_t)((uint16_t)(x_after_plot_shallow + x_step) - 1);
                                                            count_3 = (uint16_t)(count_3 - 1);
                                                            past_drop_end = (line_error_shallow >= dy_abs);
                                                            line_error_shallow = (uint16_t)(line_error_shallow - dy_abs);
                                                            if (count_3 != 0) {
                                                                continue;
                                                            } else {
                                                                break;
                                                            }
                                                        }
                                                    }
                                                    seed = fx_pull(stack);
                                                    drop_at = fx_pull(stack);
                                                    saved_loop = fx_pull(stack);
                                                    count = fx_pull(stack);
                                                    continue;
                                                }
                                            } else {
                                                drop_at = (uint16_t)(drop_extra_ptr + 2);
                                                past_drop_end = (0x0100 >= drop_z_moved);
                                                if (!((uint16_t)((uint16_t)(0 - drop_z_moved) + 0x0100) & 0x8000)) {
                                                    fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                    screen_x_next = (uint16_t)(screen_x + 1);
                                                    fx_plot((uint8_t)screen_x_next, (uint8_t)screen_y_top);
                                                    screen_y_below = (uint16_t)(screen_y_top + 1);
                                                    screen_x_back = (uint16_t)(screen_x_next - 1);
                                                    fx_plot((uint8_t)screen_x_back, (uint8_t)screen_y_below);
                                                    fx_plot((uint8_t)(uint16_t)(screen_x_back + 1), (uint8_t)screen_y_below);
                                                    continue;
                                                } else {
                                                    fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                    continue;
                                                }
                                            }
                                        } else {
                                            drop_life_ptr = (uint16_t)(drop_extra_ptr - 8);
                                            past_drop_end = ((uint32_t)drop_life_ptr + 0 > 0xFFFF);
                                            fx_stb(drop_life_ptr, (uint8_t)1);
                                            drop_at = (uint16_t)(drop_extra_ptr + 2);
                                            continue;
                                        }
                                    } else {
                                        past_drop_end = 1;
                                        fx_stw(drop_extra_ptr, 0);
                                        drop_at = (uint16_t)(drop_extra_ptr + 2);
                                        continue;
                                    }
                                } else {
                                    past_drop_end = 1;
                                    fx_stw(drop_extra_ptr, 0);
                                    drop_at = (uint16_t)(drop_extra_ptr + 2);
                                    continue;
                                }
                            } else {
                                past_drop_end = 1;
                                fx_stw(drop_extra_ptr, 0);
                                drop_at = (uint16_t)(drop_extra_ptr + 2);
                                continue;
                            }
                        } else {
                            drop_z_bound = 0;
                            fx_sbk(drop_z_bound);
                            past_drop_end = 1;
                            fx_stw(drop_extra_ptr, 0);
                            drop_at = (uint16_t)(drop_extra_ptr + 2);
                            continue;
                        }
                    } else {
                        drop_z_bound = 0x01FF;
                        fx_sbk(drop_z_bound);
                        past_drop_end = 1;
                        fx_stw(drop_extra_ptr, 0);
                        drop_at = (uint16_t)(drop_extra_ptr + 2);
                        continue;
                    }
                } else {
                    fx_stb(drop_at, (uint8_t)(uint16_t)(drop_life_less_one + 2));
                    past_drop_end = ((uint32_t)drop_at + 0x000A > 0xFFFF);
                    drop_at = (uint16_t)(drop_at + 0x000A);
                    continue;
                }
            }
        } else {
            break;
        }
    }
    fx_stw(0x01D0, seed);
    fx_unspill(stack);
    return (FxWeatherUnusedLoopResult){1};
}


/*
 * @routine 01:BF35 FxWeatherUnusedClear
 * WHAT IT DOES: Sets the word four bytes before R8 (a drop's) to 0 and
 * goes on with the drop loop (01:BF2A); nothing reaches it (it follows
 * an unconditional BRA), so a direct call is its only way in.
 * HOW: IWT R2,#0; BRA to the store at 01:BF3E (R8 - 10 + 6), R8 kept.
 * Entry: R8 = the drop; the drop loop's R12, R13, R10. Exit: R1 = R8;
 * R2 = 0; then as FxWeatherDraw.
 */
FxWeatherUnusedClearResult FxWeatherUnusedClear(uint16_t seed, uint16_t drop, uint16_t *stack, uint16_t weather_kind, uint16_t count)
{
    int run_lBDDD = 0;
    int run_lBE0C = 0;
    uint16_t dx;
    uint16_t dy;
    uint16_t me_z;
    uint16_t drop_x;
    uint16_t drop_y;
    uint16_t drop_z;
    uint16_t dx_abs;
    uint16_t dy_abs;
    uint16_t x_step;
    uint16_t y_step;
    uint16_t count_2;
    uint16_t count_3;
    uint16_t drop_at;
    uint16_t haze_x8;
    uint16_t clip_top;
    uint16_t new_life;
    uint16_t screen_x;
    uint16_t clip_left;
    uint16_t drop_life;
    uint16_t clip_right;
    uint16_t drop_x_ptr;
    uint16_t drop_y_ptr;
    uint16_t drop_z_ptr;
    uint16_t new_drop_z;
    uint16_t prev_y_ptr;
    uint16_t saved_loop;
    uint16_t seed_sum_2;
    uint16_t seed_sum_3;
    uint16_t clip_bottom;
    uint16_t drop_colour;
    uint16_t dx_minus_dy;
    uint16_t recip_scale;
    uint16_t seed_next_1;
    uint16_t drop_x_moved;
    uint16_t drop_y_moved;
    uint16_t drop_z_bound;
    uint16_t drop_z_moved;
    uint16_t screen_y_top;
    int seed_carry_2;
    uint16_t drift_x_delta;
    uint16_t drop_life_ptr;
    uint16_t drop_life_raw;
    uint16_t drop_minus_10;
    uint16_t drop_x_biased;
    uint16_t drop_y_biased;
    int past_drop_end;
    uint16_t prev_screen_y;
    uint16_t screen_x_back;
    uint16_t screen_x_next;
    uint32_t screen_x_prod;
    uint32_t screen_y_prod;
    uint16_t view_center_x;
    uint16_t view_center_y;
    uint16_t drop_extra_ptr;
    uint16_t drop_x_clamped;
    uint16_t drop_x_ptr_new;
    uint16_t drop_x_wrapped;
    uint16_t drop_y_clamped;
    uint16_t drop_y_ptr_new;
    uint16_t drop_y_wrapped;
    uint16_t drop_z_ptr_new;
    uint16_t recip_low_byte;
    uint16_t screen_y_below;
    uint16_t seed_rotated_2;
    uint16_t seed_rotated_3;
    uint16_t weather_colour;
    uint16_t drop_colour_ptr;
    uint16_t drop_x_over_max;
    uint16_t drop_y_over_max;
    uint16_t prev_screen_pos;
    uint16_t drop_x_low_bound;
    uint16_t drop_y_low_bound;
    uint16_t half_dx_minus_dy;
    uint16_t half_dy_minus_dx;
    uint16_t line_error_steep;
    uint16_t recip_table_addr;
    uint16_t perspective_index;
    uint16_t view_center_y_new;
    uint16_t drop_extra_ptr_new;
    uint16_t drop_life_less_one;
    uint16_t line_error_shallow;
    uint16_t x_after_plot_steep;
    uint16_t drop_colour_ptr_move;
    uint16_t x_after_plot_shallow;
    uint16_t drop_z_ptr_after_x_reset;
    uint16_t drop_extra_ptr_after_y_reset;

    drop_minus_10 = (uint16_t)(drop - 0x000A);
    past_drop_end = ((uint32_t)drop_minus_10 + 6 > 0xFFFF);
    fx_stw((uint16_t)(drop_minus_10 + FXREC_ALL_PREV), 0);
    drop_at = drop;
    for (;;) {
        count = (uint16_t)(count - 1);
        if (count != 0) {
            drop_life_raw = fx_ldb(drop_at);
            drop_life = (uint16_t)(int16_t)(int8_t)drop_life_raw;
            drop_life_less_one = (uint16_t)(drop_life - 1);
            if (drop_life == 0) {
                new_life = fx_ldw(0x26FC);
                fx_stb(drop_at, (uint8_t)new_life);
                drop_colour_ptr = (uint16_t)(drop_at + 1);
                seed_rotated_2 = (uint16_t)((uint16_t)(seed << 8 | seed >> 8) >> 1 | past_drop_end << 15);
                seed_sum_2 = (uint16_t)(seed_rotated_2 + seed);
                seed_carry_2 = ((uint32_t)seed_rotated_2 + seed > 0xFFFF);
                seed_next_1 = (uint16_t)(seed_sum_2 + seed + seed_carry_2);
                weather_colour = fx_ldw(FX_WEATHER_COLOUR);
                fx_stb(drop_colour_ptr, (uint8_t)weather_colour);
                drop_x_ptr_new = (uint16_t)(drop_colour_ptr + 1);
                new_drop_z = (weather_colour & 0x01FF);
                seed_rotated_3 = (uint16_t)((uint16_t)(seed_next_1 << 8 | seed_next_1 >> 8) >> 1 | ((uint32_t)seed_sum_2 + seed + seed_carry_2 > 0xFFFF) << 15);
                seed_sum_3 = (uint16_t)(seed_rotated_3 + seed_next_1);
                seed = (uint16_t)(seed_sum_3 + seed_next_1 + ((uint32_t)seed_rotated_3 + seed_next_1 > 0xFFFF));
                fx_stw(drop_x_ptr_new, (uint16_t)((seed_sum_3 & 0x03FF) - 0x01FF));
                drop_y_ptr_new = (uint16_t)(drop_x_ptr_new + 2);
                view_center_y_new = fx_ldw(0x0036);
                fx_stw(drop_y_ptr_new, (uint16_t)((uint16_t)~(uint16_t)(fx_div2(new_drop_z) + view_center_y_new) + 1));
                drop_z_ptr_new = (uint16_t)(drop_y_ptr_new + 2);
                fx_stw(drop_z_ptr_new, new_drop_z);
                drop_extra_ptr_new = (uint16_t)(drop_z_ptr_new + 2);
                past_drop_end = 1;
                fx_stw(drop_extra_ptr_new, 0);
                drop_at = (uint16_t)(drop_extra_ptr_new + 2);
                continue;
            } else {
                if (!(drop_life_less_one & 0x8000)) {
                    fx_stb(drop_at, (uint8_t)drop_life_less_one);
                    drop_colour_ptr_move = (uint16_t)(drop_at + 1);
                    drop_colour = fx_ldb(drop_colour_ptr_move);
                    drop_x_ptr = (uint16_t)(drop_colour_ptr_move + 1);
                    fx_color((uint8_t)drop_colour);
                    drift_x_delta = fx_ldw(0x26F6);
                    drop_x = fx_ldw(drop_x_ptr);
                    drop_y_ptr = (uint16_t)(drop_x_ptr + 2);
                    drop_x_moved = (uint16_t)(drop_x + drift_x_delta);
                    if (!(drop_x_moved & 0x8000)) {
                        drop_x_over_max = (uint16_t)(drop_x_moved - 0x01FF);
                        drop_x_wrapped = (uint16_t)(drop_x_over_max + 0x01FF);
                        if (drop_x_over_max & 0x8000) {
                            run_lBDDD = 1;
                        } else {
                            drop_x_low_bound = (uint16_t)((uint16_t)~0x01FF + 1);
                            fx_sbk(drop_x_low_bound);
                            drop_z_ptr_after_x_reset = (uint16_t)(drop_y_ptr + 4);
                            past_drop_end = 1;
                            fx_stw(drop_z_ptr_after_x_reset, 0);
                            drop_at = (uint16_t)(drop_z_ptr_after_x_reset + 2);
                            continue;
                        }
                    } else {
                        drop_x_biased = (uint16_t)(drop_x_moved + 0x01FF);
                        drop_x_wrapped = (uint16_t)(drop_x_biased - 0x01FF);
                        if (!(drop_x_biased & 0x8000)) {
                            run_lBDDD = 1;
                        } else {
                            drop_x_low_bound = (uint16_t)(0 + 0x01FE);
                            fx_sbk(drop_x_low_bound);
                            drop_z_ptr_after_x_reset = (uint16_t)(drop_y_ptr + 4);
                            past_drop_end = 1;
                            fx_stw(drop_z_ptr_after_x_reset, 0);
                            drop_at = (uint16_t)(drop_z_ptr_after_x_reset + 2);
                            continue;
                        }
                    }
                    fx_sbk(drop_x_wrapped);
                    drop_x_clamped = drop_x_wrapped;
                    haze_x8 = fx_ldw(FX_HAZE_X8);
                    drop_y = fx_ldw(drop_y_ptr);
                    drop_z_ptr = (uint16_t)(drop_y_ptr + 2);
                    drop_y_moved = (uint16_t)(drop_y + haze_x8);
                    if (!(drop_y_moved & 0x8000)) {
                        drop_y_over_max = (uint16_t)(drop_y_moved - 0x00FF);
                        drop_y_wrapped = (uint16_t)(drop_y_over_max + 0x00FF);
                        if (drop_y_over_max & 0x8000) {
                            run_lBE0C = 1;
                        } else {
                            drop_y_low_bound = (uint16_t)((uint16_t)~0x00FF + 1);
                            fx_sbk(drop_y_low_bound);
                            drop_extra_ptr_after_y_reset = (uint16_t)(drop_z_ptr + 2);
                            past_drop_end = 1;
                            fx_stw(drop_extra_ptr_after_y_reset, 0);
                            drop_at = (uint16_t)(drop_extra_ptr_after_y_reset + 2);
                            continue;
                        }
                    } else {
                        drop_y_biased = (uint16_t)(drop_y_moved + 0x00FF);
                        drop_y_wrapped = (uint16_t)(drop_y_biased - 0x00FF);
                        if (!(drop_y_biased & 0x8000)) {
                            run_lBE0C = 1;
                        } else {
                            drop_y_low_bound = (uint16_t)(0 + 0x00FE);
                            fx_sbk(drop_y_low_bound);
                            drop_extra_ptr_after_y_reset = (uint16_t)(drop_z_ptr + 2);
                            past_drop_end = 1;
                            fx_stw(drop_extra_ptr_after_y_reset, 0);
                            drop_at = (uint16_t)(drop_extra_ptr_after_y_reset + 2);
                            continue;
                        }
                    }
                    fx_sbk(drop_y_wrapped);
                    drop_y_clamped = drop_y_wrapped;
                    me_z = fx_ldw(FX_ME_Z);
                    drop_z = fx_ldw(drop_z_ptr);
                    drop_extra_ptr = (uint16_t)(drop_z_ptr + 2);
                    drop_z_moved = (uint16_t)(drop_z + me_z);
                    if (!(drop_z_moved & 0x8000)) {
                        if ((uint16_t)(drop_z_moved - 0x01FF) & 0x8000) {
                            fx_sbk(drop_z_moved);
                            perspective_index = (uint16_t)(drop_z_moved + 0x012C);
                            recip_table_addr = (uint16_t)((uint16_t)(perspective_index + perspective_index) - 0x7694);
                            fx_r14(recip_table_addr);
                            recip_low_byte = fx_getb();
                            fx_r14((uint16_t)(recip_table_addr + 1));
                            recip_scale = (uint16_t)((recip_low_byte & 0xFF) | fx_getb() << 8);
                            screen_x_prod = (uint32_t)((int32_t)(int16_t)drop_x_clamped * (int16_t)recip_scale);
                            view_center_x = fx_ldw(0x0034);
                            screen_x = (uint16_t)(view_center_x + (uint16_t)((uint16_t)(screen_x_prod >> 16) << 1 | (int)(screen_x_prod >> 15 & 1)));
                            clip_left = fx_ldw(0x0038);
                            if ((uint16_t)(clip_left - screen_x) & 0x8000) {
                                clip_right = fx_ldw(0x003A);
                                if (!((uint16_t)((uint16_t)(clip_right - screen_x) - 2) & 0x8000)) {
                                    screen_y_prod = (uint32_t)((int32_t)(int16_t)drop_y_clamped * (int16_t)recip_scale);
                                    view_center_y = fx_ldw(0x0036);
                                    screen_y_top = (uint16_t)((uint16_t)(view_center_y + (uint16_t)((uint16_t)(screen_y_prod >> 16) << 1 | (int)(screen_y_prod >> 15 & 1))) - 0x0014);
                                    clip_top = fx_ldw(0x003C);
                                    if ((uint16_t)(clip_top - screen_y_top) & 0x8000) {
                                        clip_bottom = fx_ldw(0x003E);
                                        if (!((uint16_t)((uint16_t)(clip_bottom - screen_y_top) - 4) & 0x8000)) {
                                            past_drop_end = (weather_kind >= 1);
                                            if (weather_kind == 1) {
                                                prev_screen_pos = fx_ldw(drop_extra_ptr);
                                                prev_screen_y = (uint16_t)(prev_screen_pos >> 8);
                                                fx_stb(drop_extra_ptr, (uint8_t)screen_x);
                                                prev_y_ptr = (uint16_t)(drop_extra_ptr + 1);
                                                fx_stb(prev_y_ptr, (uint8_t)screen_y_top);
                                                drop_at = (uint16_t)(prev_y_ptr + 1);
                                                if (prev_screen_pos == 0) {
                                                    continue;
                                                } else {
                                                    fx_push(stack, count);
                                                    fx_push(stack, saved_loop);
                                                    fx_push(stack, drop_at);
                                                    fx_push(stack, seed);
                                                    x_step = 1;
                                                    dx = (uint16_t)((uint16_t)(prev_screen_pos & 0xFF) - screen_x);
                                                    dx_abs = dx;
                                                    if (!(dx & 0x8000)) {
                                                    } else {
                                                        x_step = 0xFFFF;
                                                        dx_abs = (uint16_t)((uint16_t)~dx_abs + 1);
                                                    }
                                                    y_step = 1;
                                                    dy = (uint16_t)(prev_screen_y - screen_y_top);
                                                    dy_abs = dy;
                                                    if (!(dy & 0x8000)) {
                                                    } else {
                                                        y_step = 0xFFFF;
                                                        dy_abs = (uint16_t)((uint16_t)~dy_abs + 1);
                                                    }
                                                    dx_minus_dy = (uint16_t)(dx_abs - dy_abs);
                                                    if (dx_minus_dy & 0x8000) {
                                                        count_2 = (uint16_t)(dy_abs + 1);
                                                        half_dy_minus_dx = (uint16_t)((uint16_t)((uint16_t)~dx_minus_dy + 1) >> 1);
                                                        past_drop_end = (half_dy_minus_dx >= dx_abs);
                                                        line_error_steep = (uint16_t)(half_dy_minus_dx - dx_abs);
                                                        for (;;) {
                                                            fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                            x_after_plot_steep = (uint16_t)(screen_x + 1);
                                                            if (past_drop_end) {
                                                            } else {
                                                                line_error_steep = (uint16_t)(line_error_steep + dy_abs);
                                                                x_after_plot_steep = (uint16_t)(x_after_plot_steep + x_step);
                                                            }
                                                            screen_x = (uint16_t)(x_after_plot_steep - 1);
                                                            screen_y_top = (uint16_t)(screen_y_top + y_step);
                                                            count_2 = (uint16_t)(count_2 - 1);
                                                            past_drop_end = (line_error_steep >= dx_abs);
                                                            line_error_steep = (uint16_t)(line_error_steep - dx_abs);
                                                            if (count_2 != 0) {
                                                                continue;
                                                            } else {
                                                                break;
                                                            }
                                                        }
                                                    } else {
                                                        count_3 = (uint16_t)(dx_abs + 1);
                                                        half_dx_minus_dy = (uint16_t)(dx_minus_dy >> 1);
                                                        past_drop_end = (half_dx_minus_dy >= dy_abs);
                                                        line_error_shallow = (uint16_t)(half_dx_minus_dy - dy_abs);
                                                        for (;;) {
                                                            fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                            x_after_plot_shallow = (uint16_t)(screen_x + 1);
                                                            if (past_drop_end) {
                                                            } else {
                                                                line_error_shallow = (uint16_t)(line_error_shallow + dx_abs);
                                                                screen_y_top = (uint16_t)(screen_y_top + y_step);
                                                            }
                                                            screen_x = (uint16_t)((uint16_t)(x_after_plot_shallow + x_step) - 1);
                                                            count_3 = (uint16_t)(count_3 - 1);
                                                            past_drop_end = (line_error_shallow >= dy_abs);
                                                            line_error_shallow = (uint16_t)(line_error_shallow - dy_abs);
                                                            if (count_3 != 0) {
                                                                continue;
                                                            } else {
                                                                break;
                                                            }
                                                        }
                                                    }
                                                    seed = fx_pull(stack);
                                                    drop_at = fx_pull(stack);
                                                    saved_loop = fx_pull(stack);
                                                    count = fx_pull(stack);
                                                    continue;
                                                }
                                            } else {
                                                drop_at = (uint16_t)(drop_extra_ptr + 2);
                                                past_drop_end = (0x0100 >= drop_z_moved);
                                                if (!((uint16_t)((uint16_t)(0 - drop_z_moved) + 0x0100) & 0x8000)) {
                                                    fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                    screen_x_next = (uint16_t)(screen_x + 1);
                                                    fx_plot((uint8_t)screen_x_next, (uint8_t)screen_y_top);
                                                    screen_y_below = (uint16_t)(screen_y_top + 1);
                                                    screen_x_back = (uint16_t)(screen_x_next - 1);
                                                    fx_plot((uint8_t)screen_x_back, (uint8_t)screen_y_below);
                                                    fx_plot((uint8_t)(uint16_t)(screen_x_back + 1), (uint8_t)screen_y_below);
                                                    continue;
                                                } else {
                                                    fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                    continue;
                                                }
                                            }
                                        } else {
                                            drop_life_ptr = (uint16_t)(drop_extra_ptr - 8);
                                            past_drop_end = ((uint32_t)drop_life_ptr + 0 > 0xFFFF);
                                            fx_stb(drop_life_ptr, (uint8_t)1);
                                            drop_at = (uint16_t)(drop_extra_ptr + 2);
                                            continue;
                                        }
                                    } else {
                                        past_drop_end = 1;
                                        fx_stw(drop_extra_ptr, 0);
                                        drop_at = (uint16_t)(drop_extra_ptr + 2);
                                        continue;
                                    }
                                } else {
                                    past_drop_end = 1;
                                    fx_stw(drop_extra_ptr, 0);
                                    drop_at = (uint16_t)(drop_extra_ptr + 2);
                                    continue;
                                }
                            } else {
                                past_drop_end = 1;
                                fx_stw(drop_extra_ptr, 0);
                                drop_at = (uint16_t)(drop_extra_ptr + 2);
                                continue;
                            }
                        } else {
                            drop_z_bound = 0;
                            fx_sbk(drop_z_bound);
                            past_drop_end = 1;
                            fx_stw(drop_extra_ptr, 0);
                            drop_at = (uint16_t)(drop_extra_ptr + 2);
                            continue;
                        }
                    } else {
                        drop_z_bound = 0x01FF;
                        fx_sbk(drop_z_bound);
                        past_drop_end = 1;
                        fx_stw(drop_extra_ptr, 0);
                        drop_at = (uint16_t)(drop_extra_ptr + 2);
                        continue;
                    }
                } else {
                    fx_stb(drop_at, (uint8_t)(uint16_t)(drop_life_less_one + 2));
                    past_drop_end = ((uint32_t)drop_at + 0x000A > 0xFFFF);
                    drop_at = (uint16_t)(drop_at + 0x000A);
                    continue;
                }
            }
        } else {
            break;
        }
    }
    fx_stw(0x01D0, seed);
    fx_unspill(stack);
    return (FxWeatherUnusedClearResult){1};
}


/*
 * @routine 01:BF3B FxWeatherUnusedFull
 * WHAT IT DOES: As FxWeatherUnusedClear with $01FF: the word four bytes
 * before R8 set to $01FF, then the drop loop (01:BF2A); nothing reaches
 * it, so a direct call is its only way in.
 * HOW: IWT R2,#$01FF; the store at 01:BF3E.
 * Entry: R8 = the drop; the drop loop's R12, R13, R10. Exit: R1 = R8;
 * R2 = $01FF; then as FxWeatherDraw.
 */
FxWeatherUnusedFullResult FxWeatherUnusedFull(uint16_t seed, uint16_t drop, uint16_t *stack, uint16_t weather_kind, uint16_t count)
{
    int run_lBDDD = 0;
    int run_lBE0C = 0;
    uint16_t dx;
    uint16_t dy;
    uint16_t me_z;
    uint16_t drop_x;
    uint16_t drop_y;
    uint16_t drop_z;
    uint16_t dx_abs;
    uint16_t dy_abs;
    uint16_t x_step;
    uint16_t y_step;
    uint16_t count_2;
    uint16_t count_3;
    uint16_t drop_at;
    uint16_t haze_x8;
    uint16_t clip_top;
    uint16_t new_life;
    uint16_t screen_x;
    uint16_t clip_left;
    uint16_t drop_life;
    uint16_t clip_right;
    uint16_t drop_x_ptr;
    uint16_t drop_y_ptr;
    uint16_t drop_z_ptr;
    uint16_t new_drop_z;
    uint16_t prev_y_ptr;
    uint16_t saved_loop;
    uint16_t seed_sum_2;
    uint16_t seed_sum_3;
    uint16_t clip_bottom;
    uint16_t drop_colour;
    uint16_t dx_minus_dy;
    uint16_t recip_scale;
    uint16_t seed_next_1;
    uint16_t drop_x_moved;
    uint16_t drop_y_moved;
    uint16_t drop_z_bound;
    uint16_t drop_z_moved;
    uint16_t screen_y_top;
    int seed_carry_2;
    uint16_t drift_x_delta;
    uint16_t drop_life_ptr;
    uint16_t drop_life_raw;
    uint16_t drop_minus_10;
    uint16_t drop_x_biased;
    uint16_t drop_y_biased;
    int past_drop_end;
    uint16_t prev_screen_y;
    uint16_t screen_x_back;
    uint16_t screen_x_next;
    uint32_t screen_x_prod;
    uint32_t screen_y_prod;
    uint16_t view_center_x;
    uint16_t view_center_y;
    uint16_t drop_extra_ptr;
    uint16_t drop_x_clamped;
    uint16_t drop_x_ptr_new;
    uint16_t drop_x_wrapped;
    uint16_t drop_y_clamped;
    uint16_t drop_y_ptr_new;
    uint16_t drop_y_wrapped;
    uint16_t drop_z_ptr_new;
    uint16_t recip_low_byte;
    uint16_t screen_y_below;
    uint16_t seed_rotated_2;
    uint16_t seed_rotated_3;
    uint16_t weather_colour;
    uint16_t drop_colour_ptr;
    uint16_t drop_x_over_max;
    uint16_t drop_y_over_max;
    uint16_t prev_screen_pos;
    uint16_t drop_x_low_bound;
    uint16_t drop_y_low_bound;
    uint16_t half_dx_minus_dy;
    uint16_t half_dy_minus_dx;
    uint16_t line_error_steep;
    uint16_t recip_table_addr;
    uint16_t perspective_index;
    uint16_t view_center_y_new;
    uint16_t drop_extra_ptr_new;
    uint16_t drop_life_less_one;
    uint16_t line_error_shallow;
    uint16_t x_after_plot_steep;
    uint16_t drop_colour_ptr_move;
    uint16_t x_after_plot_shallow;
    uint16_t drop_z_ptr_after_x_reset;
    uint16_t drop_extra_ptr_after_y_reset;

    drop_minus_10 = (uint16_t)(drop - 0x000A);
    past_drop_end = ((uint32_t)drop_minus_10 + 6 > 0xFFFF);
    fx_stw((uint16_t)(drop_minus_10 + FXREC_ALL_PREV), 0x01FF);
    drop_at = drop;
    for (;;) {
        count = (uint16_t)(count - 1);
        if (count != 0) {
            drop_life_raw = fx_ldb(drop_at);
            drop_life = (uint16_t)(int16_t)(int8_t)drop_life_raw;
            drop_life_less_one = (uint16_t)(drop_life - 1);
            if (drop_life == 0) {
                new_life = fx_ldw(0x26FC);
                fx_stb(drop_at, (uint8_t)new_life);
                drop_colour_ptr = (uint16_t)(drop_at + 1);
                seed_rotated_2 = (uint16_t)((uint16_t)(seed << 8 | seed >> 8) >> 1 | past_drop_end << 15);
                seed_sum_2 = (uint16_t)(seed_rotated_2 + seed);
                seed_carry_2 = ((uint32_t)seed_rotated_2 + seed > 0xFFFF);
                seed_next_1 = (uint16_t)(seed_sum_2 + seed + seed_carry_2);
                weather_colour = fx_ldw(FX_WEATHER_COLOUR);
                fx_stb(drop_colour_ptr, (uint8_t)weather_colour);
                drop_x_ptr_new = (uint16_t)(drop_colour_ptr + 1);
                new_drop_z = (weather_colour & 0x01FF);
                seed_rotated_3 = (uint16_t)((uint16_t)(seed_next_1 << 8 | seed_next_1 >> 8) >> 1 | ((uint32_t)seed_sum_2 + seed + seed_carry_2 > 0xFFFF) << 15);
                seed_sum_3 = (uint16_t)(seed_rotated_3 + seed_next_1);
                seed = (uint16_t)(seed_sum_3 + seed_next_1 + ((uint32_t)seed_rotated_3 + seed_next_1 > 0xFFFF));
                fx_stw(drop_x_ptr_new, (uint16_t)((seed_sum_3 & 0x03FF) - 0x01FF));
                drop_y_ptr_new = (uint16_t)(drop_x_ptr_new + 2);
                view_center_y_new = fx_ldw(0x0036);
                fx_stw(drop_y_ptr_new, (uint16_t)((uint16_t)~(uint16_t)(fx_div2(new_drop_z) + view_center_y_new) + 1));
                drop_z_ptr_new = (uint16_t)(drop_y_ptr_new + 2);
                fx_stw(drop_z_ptr_new, new_drop_z);
                drop_extra_ptr_new = (uint16_t)(drop_z_ptr_new + 2);
                past_drop_end = 1;
                fx_stw(drop_extra_ptr_new, 0);
                drop_at = (uint16_t)(drop_extra_ptr_new + 2);
                continue;
            } else {
                if (!(drop_life_less_one & 0x8000)) {
                    fx_stb(drop_at, (uint8_t)drop_life_less_one);
                    drop_colour_ptr_move = (uint16_t)(drop_at + 1);
                    drop_colour = fx_ldb(drop_colour_ptr_move);
                    drop_x_ptr = (uint16_t)(drop_colour_ptr_move + 1);
                    fx_color((uint8_t)drop_colour);
                    drift_x_delta = fx_ldw(0x26F6);
                    drop_x = fx_ldw(drop_x_ptr);
                    drop_y_ptr = (uint16_t)(drop_x_ptr + 2);
                    drop_x_moved = (uint16_t)(drop_x + drift_x_delta);
                    if (!(drop_x_moved & 0x8000)) {
                        drop_x_over_max = (uint16_t)(drop_x_moved - 0x01FF);
                        drop_x_wrapped = (uint16_t)(drop_x_over_max + 0x01FF);
                        if (drop_x_over_max & 0x8000) {
                            run_lBDDD = 1;
                        } else {
                            drop_x_low_bound = (uint16_t)((uint16_t)~0x01FF + 1);
                            fx_sbk(drop_x_low_bound);
                            drop_z_ptr_after_x_reset = (uint16_t)(drop_y_ptr + 4);
                            past_drop_end = 1;
                            fx_stw(drop_z_ptr_after_x_reset, 0);
                            drop_at = (uint16_t)(drop_z_ptr_after_x_reset + 2);
                            continue;
                        }
                    } else {
                        drop_x_biased = (uint16_t)(drop_x_moved + 0x01FF);
                        drop_x_wrapped = (uint16_t)(drop_x_biased - 0x01FF);
                        if (!(drop_x_biased & 0x8000)) {
                            run_lBDDD = 1;
                        } else {
                            drop_x_low_bound = (uint16_t)(0 + 0x01FE);
                            fx_sbk(drop_x_low_bound);
                            drop_z_ptr_after_x_reset = (uint16_t)(drop_y_ptr + 4);
                            past_drop_end = 1;
                            fx_stw(drop_z_ptr_after_x_reset, 0);
                            drop_at = (uint16_t)(drop_z_ptr_after_x_reset + 2);
                            continue;
                        }
                    }
                    fx_sbk(drop_x_wrapped);
                    drop_x_clamped = drop_x_wrapped;
                    haze_x8 = fx_ldw(FX_HAZE_X8);
                    drop_y = fx_ldw(drop_y_ptr);
                    drop_z_ptr = (uint16_t)(drop_y_ptr + 2);
                    drop_y_moved = (uint16_t)(drop_y + haze_x8);
                    if (!(drop_y_moved & 0x8000)) {
                        drop_y_over_max = (uint16_t)(drop_y_moved - 0x00FF);
                        drop_y_wrapped = (uint16_t)(drop_y_over_max + 0x00FF);
                        if (drop_y_over_max & 0x8000) {
                            run_lBE0C = 1;
                        } else {
                            drop_y_low_bound = (uint16_t)((uint16_t)~0x00FF + 1);
                            fx_sbk(drop_y_low_bound);
                            drop_extra_ptr_after_y_reset = (uint16_t)(drop_z_ptr + 2);
                            past_drop_end = 1;
                            fx_stw(drop_extra_ptr_after_y_reset, 0);
                            drop_at = (uint16_t)(drop_extra_ptr_after_y_reset + 2);
                            continue;
                        }
                    } else {
                        drop_y_biased = (uint16_t)(drop_y_moved + 0x00FF);
                        drop_y_wrapped = (uint16_t)(drop_y_biased - 0x00FF);
                        if (!(drop_y_biased & 0x8000)) {
                            run_lBE0C = 1;
                        } else {
                            drop_y_low_bound = (uint16_t)(0 + 0x00FE);
                            fx_sbk(drop_y_low_bound);
                            drop_extra_ptr_after_y_reset = (uint16_t)(drop_z_ptr + 2);
                            past_drop_end = 1;
                            fx_stw(drop_extra_ptr_after_y_reset, 0);
                            drop_at = (uint16_t)(drop_extra_ptr_after_y_reset + 2);
                            continue;
                        }
                    }
                    fx_sbk(drop_y_wrapped);
                    drop_y_clamped = drop_y_wrapped;
                    me_z = fx_ldw(FX_ME_Z);
                    drop_z = fx_ldw(drop_z_ptr);
                    drop_extra_ptr = (uint16_t)(drop_z_ptr + 2);
                    drop_z_moved = (uint16_t)(drop_z + me_z);
                    if (!(drop_z_moved & 0x8000)) {
                        if ((uint16_t)(drop_z_moved - 0x01FF) & 0x8000) {
                            fx_sbk(drop_z_moved);
                            perspective_index = (uint16_t)(drop_z_moved + 0x012C);
                            recip_table_addr = (uint16_t)((uint16_t)(perspective_index + perspective_index) - 0x7694);
                            fx_r14(recip_table_addr);
                            recip_low_byte = fx_getb();
                            fx_r14((uint16_t)(recip_table_addr + 1));
                            recip_scale = (uint16_t)((recip_low_byte & 0xFF) | fx_getb() << 8);
                            screen_x_prod = (uint32_t)((int32_t)(int16_t)drop_x_clamped * (int16_t)recip_scale);
                            view_center_x = fx_ldw(0x0034);
                            screen_x = (uint16_t)(view_center_x + (uint16_t)((uint16_t)(screen_x_prod >> 16) << 1 | (int)(screen_x_prod >> 15 & 1)));
                            clip_left = fx_ldw(0x0038);
                            if ((uint16_t)(clip_left - screen_x) & 0x8000) {
                                clip_right = fx_ldw(0x003A);
                                if (!((uint16_t)((uint16_t)(clip_right - screen_x) - 2) & 0x8000)) {
                                    screen_y_prod = (uint32_t)((int32_t)(int16_t)drop_y_clamped * (int16_t)recip_scale);
                                    view_center_y = fx_ldw(0x0036);
                                    screen_y_top = (uint16_t)((uint16_t)(view_center_y + (uint16_t)((uint16_t)(screen_y_prod >> 16) << 1 | (int)(screen_y_prod >> 15 & 1))) - 0x0014);
                                    clip_top = fx_ldw(0x003C);
                                    if ((uint16_t)(clip_top - screen_y_top) & 0x8000) {
                                        clip_bottom = fx_ldw(0x003E);
                                        if (!((uint16_t)((uint16_t)(clip_bottom - screen_y_top) - 4) & 0x8000)) {
                                            past_drop_end = (weather_kind >= 1);
                                            if (weather_kind == 1) {
                                                prev_screen_pos = fx_ldw(drop_extra_ptr);
                                                prev_screen_y = (uint16_t)(prev_screen_pos >> 8);
                                                fx_stb(drop_extra_ptr, (uint8_t)screen_x);
                                                prev_y_ptr = (uint16_t)(drop_extra_ptr + 1);
                                                fx_stb(prev_y_ptr, (uint8_t)screen_y_top);
                                                drop_at = (uint16_t)(prev_y_ptr + 1);
                                                if (prev_screen_pos == 0) {
                                                    continue;
                                                } else {
                                                    fx_push(stack, count);
                                                    fx_push(stack, saved_loop);
                                                    fx_push(stack, drop_at);
                                                    fx_push(stack, seed);
                                                    x_step = 1;
                                                    dx = (uint16_t)((uint16_t)(prev_screen_pos & 0xFF) - screen_x);
                                                    dx_abs = dx;
                                                    if (!(dx & 0x8000)) {
                                                    } else {
                                                        x_step = 0xFFFF;
                                                        dx_abs = (uint16_t)((uint16_t)~dx_abs + 1);
                                                    }
                                                    y_step = 1;
                                                    dy = (uint16_t)(prev_screen_y - screen_y_top);
                                                    dy_abs = dy;
                                                    if (!(dy & 0x8000)) {
                                                    } else {
                                                        y_step = 0xFFFF;
                                                        dy_abs = (uint16_t)((uint16_t)~dy_abs + 1);
                                                    }
                                                    dx_minus_dy = (uint16_t)(dx_abs - dy_abs);
                                                    if (dx_minus_dy & 0x8000) {
                                                        count_2 = (uint16_t)(dy_abs + 1);
                                                        half_dy_minus_dx = (uint16_t)((uint16_t)((uint16_t)~dx_minus_dy + 1) >> 1);
                                                        past_drop_end = (half_dy_minus_dx >= dx_abs);
                                                        line_error_steep = (uint16_t)(half_dy_minus_dx - dx_abs);
                                                        for (;;) {
                                                            fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                            x_after_plot_steep = (uint16_t)(screen_x + 1);
                                                            if (past_drop_end) {
                                                            } else {
                                                                line_error_steep = (uint16_t)(line_error_steep + dy_abs);
                                                                x_after_plot_steep = (uint16_t)(x_after_plot_steep + x_step);
                                                            }
                                                            screen_x = (uint16_t)(x_after_plot_steep - 1);
                                                            screen_y_top = (uint16_t)(screen_y_top + y_step);
                                                            count_2 = (uint16_t)(count_2 - 1);
                                                            past_drop_end = (line_error_steep >= dx_abs);
                                                            line_error_steep = (uint16_t)(line_error_steep - dx_abs);
                                                            if (count_2 != 0) {
                                                                continue;
                                                            } else {
                                                                break;
                                                            }
                                                        }
                                                    } else {
                                                        count_3 = (uint16_t)(dx_abs + 1);
                                                        half_dx_minus_dy = (uint16_t)(dx_minus_dy >> 1);
                                                        past_drop_end = (half_dx_minus_dy >= dy_abs);
                                                        line_error_shallow = (uint16_t)(half_dx_minus_dy - dy_abs);
                                                        for (;;) {
                                                            fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                            x_after_plot_shallow = (uint16_t)(screen_x + 1);
                                                            if (past_drop_end) {
                                                            } else {
                                                                line_error_shallow = (uint16_t)(line_error_shallow + dx_abs);
                                                                screen_y_top = (uint16_t)(screen_y_top + y_step);
                                                            }
                                                            screen_x = (uint16_t)((uint16_t)(x_after_plot_shallow + x_step) - 1);
                                                            count_3 = (uint16_t)(count_3 - 1);
                                                            past_drop_end = (line_error_shallow >= dy_abs);
                                                            line_error_shallow = (uint16_t)(line_error_shallow - dy_abs);
                                                            if (count_3 != 0) {
                                                                continue;
                                                            } else {
                                                                break;
                                                            }
                                                        }
                                                    }
                                                    seed = fx_pull(stack);
                                                    drop_at = fx_pull(stack);
                                                    saved_loop = fx_pull(stack);
                                                    count = fx_pull(stack);
                                                    continue;
                                                }
                                            } else {
                                                drop_at = (uint16_t)(drop_extra_ptr + 2);
                                                past_drop_end = (0x0100 >= drop_z_moved);
                                                if (!((uint16_t)((uint16_t)(0 - drop_z_moved) + 0x0100) & 0x8000)) {
                                                    fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                    screen_x_next = (uint16_t)(screen_x + 1);
                                                    fx_plot((uint8_t)screen_x_next, (uint8_t)screen_y_top);
                                                    screen_y_below = (uint16_t)(screen_y_top + 1);
                                                    screen_x_back = (uint16_t)(screen_x_next - 1);
                                                    fx_plot((uint8_t)screen_x_back, (uint8_t)screen_y_below);
                                                    fx_plot((uint8_t)(uint16_t)(screen_x_back + 1), (uint8_t)screen_y_below);
                                                    continue;
                                                } else {
                                                    fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                    continue;
                                                }
                                            }
                                        } else {
                                            drop_life_ptr = (uint16_t)(drop_extra_ptr - 8);
                                            past_drop_end = ((uint32_t)drop_life_ptr + 0 > 0xFFFF);
                                            fx_stb(drop_life_ptr, (uint8_t)1);
                                            drop_at = (uint16_t)(drop_extra_ptr + 2);
                                            continue;
                                        }
                                    } else {
                                        past_drop_end = 1;
                                        fx_stw(drop_extra_ptr, 0);
                                        drop_at = (uint16_t)(drop_extra_ptr + 2);
                                        continue;
                                    }
                                } else {
                                    past_drop_end = 1;
                                    fx_stw(drop_extra_ptr, 0);
                                    drop_at = (uint16_t)(drop_extra_ptr + 2);
                                    continue;
                                }
                            } else {
                                past_drop_end = 1;
                                fx_stw(drop_extra_ptr, 0);
                                drop_at = (uint16_t)(drop_extra_ptr + 2);
                                continue;
                            }
                        } else {
                            drop_z_bound = 0;
                            fx_sbk(drop_z_bound);
                            past_drop_end = 1;
                            fx_stw(drop_extra_ptr, 0);
                            drop_at = (uint16_t)(drop_extra_ptr + 2);
                            continue;
                        }
                    } else {
                        drop_z_bound = 0x01FF;
                        fx_sbk(drop_z_bound);
                        past_drop_end = 1;
                        fx_stw(drop_extra_ptr, 0);
                        drop_at = (uint16_t)(drop_extra_ptr + 2);
                        continue;
                    }
                } else {
                    fx_stb(drop_at, (uint8_t)(uint16_t)(drop_life_less_one + 2));
                    past_drop_end = ((uint32_t)drop_at + 0x000A > 0xFFFF);
                    drop_at = (uint16_t)(drop_at + 0x000A);
                    continue;
                }
            }
        } else {
            break;
        }
    }
    fx_stw(0x01D0, seed);
    fx_unspill(stack);
    return (FxWeatherUnusedFullResult){1};
}


/*
 * @routine 01:BF4D FxWeatherDropNext
 * WHAT IT DOES: The ends of FxWeatherDraw's drop loop: a drop's last
 * word cleared (01:BF4D) or its state byte set to 1 (01:BF53), then the
 * next drop (01:BF2A); after the last, the seed kept at $01D0 and the
 * return (01:BF65).
 * HOW: Reached by jump from the drop's moves; returns by pulling R15
 * for FxWeatherDraw.
 * Entry: as FxWeatherDraw leaves them in its drop loop.
 * Exit: as FxWeatherDraw.
 */
FxWeatherDropNextResult FxWeatherDropNext(uint16_t seed, uint16_t drop_ptr, uint16_t *stack, uint16_t weather_kind, uint16_t count)
{
    int run_lBDDD = 0;
    int run_lBE0C = 0;
    int escape = 0;                    /* a way out of loops within loops */
    uint16_t dx;
    uint16_t dy;
    uint16_t me_z;
    uint16_t drop_x;
    uint16_t drop_y;
    uint16_t drop_z;
    uint16_t dx_abs;
    uint16_t dy_abs;
    uint16_t x_step;
    uint16_t y_step;
    uint16_t count_2;
    uint16_t count_3;
    uint16_t drop_at;
    uint16_t haze_x8;
    uint16_t clip_top;
    uint16_t new_life;
    uint16_t screen_x;
    uint16_t clip_left;
    uint16_t drop_life;
    uint16_t clip_right;
    uint16_t drop_x_ptr;
    uint16_t drop_y_ptr;
    uint16_t drop_z_ptr;
    uint16_t new_drop_z;
    uint16_t prev_y_ptr;
    uint16_t saved_loop;
    uint16_t seed_sum_2;
    uint16_t seed_sum_3;
    uint16_t clip_bottom;
    uint16_t drop_colour;
    uint16_t dx_minus_dy;
    uint16_t recip_scale;
    uint16_t seed_next_1;
    uint16_t drop_x_moved;
    uint16_t drop_y_moved;
    uint16_t drop_z_bound;
    uint16_t drop_z_moved;
    uint16_t screen_y_top;
    int seed_carry_2;
    uint16_t drift_x_delta;
    uint16_t drop_life_ptr;
    uint16_t drop_life_raw;
    uint16_t drop_x_biased;
    uint16_t drop_y_biased;
    int past_drop_end;
    uint16_t prev_screen_y;
    uint16_t screen_x_back;
    uint16_t screen_x_next;
    uint32_t screen_x_prod;
    uint32_t screen_y_prod;
    uint16_t view_center_x;
    uint16_t view_center_y;
    uint16_t drop_x_clamped;
    uint16_t drop_x_ptr_new;
    uint16_t drop_x_wrapped;
    uint16_t drop_y_clamped;
    uint16_t drop_y_ptr_new;
    uint16_t drop_y_wrapped;
    uint16_t drop_z_ptr_new;
    uint16_t recip_low_byte;
    uint16_t screen_y_below;
    uint16_t seed_rotated_2;
    uint16_t seed_rotated_3;
    uint16_t weather_colour;
    uint16_t drop_colour_ptr;
    uint16_t drop_x_over_max;
    uint16_t drop_y_over_max;
    uint16_t prev_screen_pos;
    uint16_t drop_x_low_bound;
    uint16_t drop_y_low_bound;
    uint16_t half_dx_minus_dy;
    uint16_t half_dy_minus_dx;
    uint16_t line_error_steep;
    uint16_t recip_table_addr;
    uint16_t perspective_index;
    uint16_t view_center_y_new;
    uint16_t drop_extra_ptr_new;
    uint16_t drop_life_less_one;
    uint16_t line_error_shallow;
    uint16_t x_after_plot_steep;
    uint16_t drop_colour_ptr_move;
    uint16_t x_after_plot_shallow;
    uint16_t drop_z_ptr_after_x_reset;
    uint16_t drop_extra_ptr_after_y_reset;

    for (;;) {
        past_drop_end = 1;
        fx_stw(drop_ptr, 0);
        drop_at = (uint16_t)(drop_ptr + 2);
        for (;;) {
            count = (uint16_t)(count - 1);
            if (count != 0) {
                drop_life_raw = fx_ldb(drop_at);
                drop_life = (uint16_t)(int16_t)(int8_t)drop_life_raw;
                drop_life_less_one = (uint16_t)(drop_life - 1);
                if (drop_life == 0) {
                    new_life = fx_ldw(0x26FC);
                    fx_stb(drop_at, (uint8_t)new_life);
                    drop_colour_ptr = (uint16_t)(drop_at + 1);
                    seed_rotated_2 = (uint16_t)((uint16_t)(seed << 8 | seed >> 8) >> 1 | past_drop_end << 15);
                    seed_sum_2 = (uint16_t)(seed_rotated_2 + seed);
                    seed_carry_2 = ((uint32_t)seed_rotated_2 + seed > 0xFFFF);
                    seed_next_1 = (uint16_t)(seed_sum_2 + seed + seed_carry_2);
                    weather_colour = fx_ldw(FX_WEATHER_COLOUR);
                    fx_stb(drop_colour_ptr, (uint8_t)weather_colour);
                    drop_x_ptr_new = (uint16_t)(drop_colour_ptr + 1);
                    new_drop_z = (weather_colour & 0x01FF);
                    seed_rotated_3 = (uint16_t)((uint16_t)(seed_next_1 << 8 | seed_next_1 >> 8) >> 1 | ((uint32_t)seed_sum_2 + seed + seed_carry_2 > 0xFFFF) << 15);
                    seed_sum_3 = (uint16_t)(seed_rotated_3 + seed_next_1);
                    seed = (uint16_t)(seed_sum_3 + seed_next_1 + ((uint32_t)seed_rotated_3 + seed_next_1 > 0xFFFF));
                    fx_stw(drop_x_ptr_new, (uint16_t)((seed_sum_3 & 0x03FF) - 0x01FF));
                    drop_y_ptr_new = (uint16_t)(drop_x_ptr_new + 2);
                    view_center_y_new = fx_ldw(0x0036);
                    fx_stw(drop_y_ptr_new, (uint16_t)((uint16_t)~(uint16_t)(fx_div2(new_drop_z) + view_center_y_new) + 1));
                    drop_z_ptr_new = (uint16_t)(drop_y_ptr_new + 2);
                    fx_stw(drop_z_ptr_new, new_drop_z);
                    drop_extra_ptr_new = (uint16_t)(drop_z_ptr_new + 2);
                    past_drop_end = 1;
                    fx_stw(drop_extra_ptr_new, 0);
                    drop_at = (uint16_t)(drop_extra_ptr_new + 2);
                    continue;
                } else {
                    if (!(drop_life_less_one & 0x8000)) {
                        fx_stb(drop_at, (uint8_t)drop_life_less_one);
                        drop_colour_ptr_move = (uint16_t)(drop_at + 1);
                        drop_colour = fx_ldb(drop_colour_ptr_move);
                        drop_x_ptr = (uint16_t)(drop_colour_ptr_move + 1);
                        fx_color((uint8_t)drop_colour);
                        drift_x_delta = fx_ldw(0x26F6);
                        drop_x = fx_ldw(drop_x_ptr);
                        drop_y_ptr = (uint16_t)(drop_x_ptr + 2);
                        drop_x_moved = (uint16_t)(drop_x + drift_x_delta);
                        if (!(drop_x_moved & 0x8000)) {
                            drop_x_over_max = (uint16_t)(drop_x_moved - 0x01FF);
                            drop_x_wrapped = (uint16_t)(drop_x_over_max + 0x01FF);
                            if (drop_x_over_max & 0x8000) {
                                run_lBDDD = 1;
                            } else {
                                drop_x_low_bound = (uint16_t)((uint16_t)~0x01FF + 1);
                                fx_sbk(drop_x_low_bound);
                                drop_z_ptr_after_x_reset = (uint16_t)(drop_y_ptr + 4);
                                past_drop_end = 1;
                                fx_stw(drop_z_ptr_after_x_reset, 0);
                                drop_at = (uint16_t)(drop_z_ptr_after_x_reset + 2);
                                continue;
                            }
                        } else {
                            drop_x_biased = (uint16_t)(drop_x_moved + 0x01FF);
                            drop_x_wrapped = (uint16_t)(drop_x_biased - 0x01FF);
                            if (!(drop_x_biased & 0x8000)) {
                                run_lBDDD = 1;
                            } else {
                                drop_x_low_bound = (uint16_t)(0 + 0x01FE);
                                fx_sbk(drop_x_low_bound);
                                drop_z_ptr_after_x_reset = (uint16_t)(drop_y_ptr + 4);
                                past_drop_end = 1;
                                fx_stw(drop_z_ptr_after_x_reset, 0);
                                drop_at = (uint16_t)(drop_z_ptr_after_x_reset + 2);
                                continue;
                            }
                        }
                        fx_sbk(drop_x_wrapped);
                        drop_x_clamped = drop_x_wrapped;
                        haze_x8 = fx_ldw(FX_HAZE_X8);
                        drop_y = fx_ldw(drop_y_ptr);
                        drop_z_ptr = (uint16_t)(drop_y_ptr + 2);
                        drop_y_moved = (uint16_t)(drop_y + haze_x8);
                        if (!(drop_y_moved & 0x8000)) {
                            drop_y_over_max = (uint16_t)(drop_y_moved - 0x00FF);
                            drop_y_wrapped = (uint16_t)(drop_y_over_max + 0x00FF);
                            if (drop_y_over_max & 0x8000) {
                                run_lBE0C = 1;
                            } else {
                                drop_y_low_bound = (uint16_t)((uint16_t)~0x00FF + 1);
                                fx_sbk(drop_y_low_bound);
                                drop_extra_ptr_after_y_reset = (uint16_t)(drop_z_ptr + 2);
                                past_drop_end = 1;
                                fx_stw(drop_extra_ptr_after_y_reset, 0);
                                drop_at = (uint16_t)(drop_extra_ptr_after_y_reset + 2);
                                continue;
                            }
                        } else {
                            drop_y_biased = (uint16_t)(drop_y_moved + 0x00FF);
                            drop_y_wrapped = (uint16_t)(drop_y_biased - 0x00FF);
                            if (!(drop_y_biased & 0x8000)) {
                                run_lBE0C = 1;
                            } else {
                                drop_y_low_bound = (uint16_t)(0 + 0x00FE);
                                fx_sbk(drop_y_low_bound);
                                drop_extra_ptr_after_y_reset = (uint16_t)(drop_z_ptr + 2);
                                past_drop_end = 1;
                                fx_stw(drop_extra_ptr_after_y_reset, 0);
                                drop_at = (uint16_t)(drop_extra_ptr_after_y_reset + 2);
                                continue;
                            }
                        }
                        fx_sbk(drop_y_wrapped);
                        drop_y_clamped = drop_y_wrapped;
                        me_z = fx_ldw(FX_ME_Z);
                        drop_z = fx_ldw(drop_z_ptr);
                        drop_ptr = (uint16_t)(drop_z_ptr + 2);
                        drop_z_moved = (uint16_t)(drop_z + me_z);
                        if (!(drop_z_moved & 0x8000)) {
                            if ((uint16_t)(drop_z_moved - 0x01FF) & 0x8000) {
                                fx_sbk(drop_z_moved);
                                perspective_index = (uint16_t)(drop_z_moved + 0x012C);
                                recip_table_addr = (uint16_t)((uint16_t)(perspective_index + perspective_index) - 0x7694);
                                fx_r14(recip_table_addr);
                                recip_low_byte = fx_getb();
                                fx_r14((uint16_t)(recip_table_addr + 1));
                                recip_scale = (uint16_t)((recip_low_byte & 0xFF) | fx_getb() << 8);
                                screen_x_prod = (uint32_t)((int32_t)(int16_t)drop_x_clamped * (int16_t)recip_scale);
                                view_center_x = fx_ldw(0x0034);
                                screen_x = (uint16_t)(view_center_x + (uint16_t)((uint16_t)(screen_x_prod >> 16) << 1 | (int)(screen_x_prod >> 15 & 1)));
                                clip_left = fx_ldw(0x0038);
                                if ((uint16_t)(clip_left - screen_x) & 0x8000) {
                                    clip_right = fx_ldw(0x003A);
                                    if (!((uint16_t)((uint16_t)(clip_right - screen_x) - 2) & 0x8000)) {
                                        screen_y_prod = (uint32_t)((int32_t)(int16_t)drop_y_clamped * (int16_t)recip_scale);
                                        view_center_y = fx_ldw(0x0036);
                                        screen_y_top = (uint16_t)((uint16_t)(view_center_y + (uint16_t)((uint16_t)(screen_y_prod >> 16) << 1 | (int)(screen_y_prod >> 15 & 1))) - 0x0014);
                                        clip_top = fx_ldw(0x003C);
                                        if ((uint16_t)(clip_top - screen_y_top) & 0x8000) {
                                            clip_bottom = fx_ldw(0x003E);
                                            if (!((uint16_t)((uint16_t)(clip_bottom - screen_y_top) - 4) & 0x8000)) {
                                                past_drop_end = (weather_kind >= 1);
                                                if (weather_kind == 1) {
                                                    prev_screen_pos = fx_ldw(drop_ptr);
                                                    prev_screen_y = (uint16_t)(prev_screen_pos >> 8);
                                                    fx_stb(drop_ptr, (uint8_t)screen_x);
                                                    prev_y_ptr = (uint16_t)(drop_ptr + 1);
                                                    fx_stb(prev_y_ptr, (uint8_t)screen_y_top);
                                                    drop_at = (uint16_t)(prev_y_ptr + 1);
                                                    if (prev_screen_pos == 0) {
                                                        continue;
                                                    } else {
                                                        fx_push(stack, count);
                                                        fx_push(stack, saved_loop);
                                                        fx_push(stack, drop_at);
                                                        fx_push(stack, seed);
                                                        x_step = 1;
                                                        dx = (uint16_t)((uint16_t)(prev_screen_pos & 0xFF) - screen_x);
                                                        dx_abs = dx;
                                                        if (!(dx & 0x8000)) {
                                                        } else {
                                                            x_step = 0xFFFF;
                                                            dx_abs = (uint16_t)((uint16_t)~dx_abs + 1);
                                                        }
                                                        y_step = 1;
                                                        dy = (uint16_t)(prev_screen_y - screen_y_top);
                                                        dy_abs = dy;
                                                        if (!(dy & 0x8000)) {
                                                        } else {
                                                            y_step = 0xFFFF;
                                                            dy_abs = (uint16_t)((uint16_t)~dy_abs + 1);
                                                        }
                                                        dx_minus_dy = (uint16_t)(dx_abs - dy_abs);
                                                        if (dx_minus_dy & 0x8000) {
                                                            count_2 = (uint16_t)(dy_abs + 1);
                                                            half_dy_minus_dx = (uint16_t)((uint16_t)((uint16_t)~dx_minus_dy + 1) >> 1);
                                                            past_drop_end = (half_dy_minus_dx >= dx_abs);
                                                            line_error_steep = (uint16_t)(half_dy_minus_dx - dx_abs);
                                                            for (;;) {
                                                                fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                                x_after_plot_steep = (uint16_t)(screen_x + 1);
                                                                if (past_drop_end) {
                                                                } else {
                                                                    line_error_steep = (uint16_t)(line_error_steep + dy_abs);
                                                                    x_after_plot_steep = (uint16_t)(x_after_plot_steep + x_step);
                                                                }
                                                                screen_x = (uint16_t)(x_after_plot_steep - 1);
                                                                screen_y_top = (uint16_t)(screen_y_top + y_step);
                                                                count_2 = (uint16_t)(count_2 - 1);
                                                                past_drop_end = (line_error_steep >= dx_abs);
                                                                line_error_steep = (uint16_t)(line_error_steep - dx_abs);
                                                                if (count_2 != 0) {
                                                                    continue;
                                                                } else {
                                                                    break;
                                                                }
                                                            }
                                                        } else {
                                                            count_3 = (uint16_t)(dx_abs + 1);
                                                            half_dx_minus_dy = (uint16_t)(dx_minus_dy >> 1);
                                                            past_drop_end = (half_dx_minus_dy >= dy_abs);
                                                            line_error_shallow = (uint16_t)(half_dx_minus_dy - dy_abs);
                                                            for (;;) {
                                                                fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                                x_after_plot_shallow = (uint16_t)(screen_x + 1);
                                                                if (past_drop_end) {
                                                                } else {
                                                                    line_error_shallow = (uint16_t)(line_error_shallow + dx_abs);
                                                                    screen_y_top = (uint16_t)(screen_y_top + y_step);
                                                                }
                                                                screen_x = (uint16_t)((uint16_t)(x_after_plot_shallow + x_step) - 1);
                                                                count_3 = (uint16_t)(count_3 - 1);
                                                                past_drop_end = (line_error_shallow >= dy_abs);
                                                                line_error_shallow = (uint16_t)(line_error_shallow - dy_abs);
                                                                if (count_3 != 0) {
                                                                    continue;
                                                                } else {
                                                                    break;
                                                                }
                                                            }
                                                        }
                                                        seed = fx_pull(stack);
                                                        drop_at = fx_pull(stack);
                                                        saved_loop = fx_pull(stack);
                                                        count = fx_pull(stack);
                                                        continue;
                                                    }
                                                } else {
                                                    drop_at = (uint16_t)(drop_ptr + 2);
                                                    past_drop_end = (0x0100 >= drop_z_moved);
                                                    if (!((uint16_t)((uint16_t)(0 - drop_z_moved) + 0x0100) & 0x8000)) {
                                                        fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                        screen_x_next = (uint16_t)(screen_x + 1);
                                                        fx_plot((uint8_t)screen_x_next, (uint8_t)screen_y_top);
                                                        screen_y_below = (uint16_t)(screen_y_top + 1);
                                                        screen_x_back = (uint16_t)(screen_x_next - 1);
                                                        fx_plot((uint8_t)screen_x_back, (uint8_t)screen_y_below);
                                                        fx_plot((uint8_t)(uint16_t)(screen_x_back + 1), (uint8_t)screen_y_below);
                                                        continue;
                                                    } else {
                                                        fx_plot((uint8_t)screen_x, (uint8_t)screen_y_top);
                                                        continue;
                                                    }
                                                }
                                            } else {
                                                drop_life_ptr = (uint16_t)(drop_ptr - 8);
                                                past_drop_end = ((uint32_t)drop_life_ptr + 0 > 0xFFFF);
                                                fx_stb(drop_life_ptr, (uint8_t)1);
                                                drop_at = (uint16_t)(drop_ptr + 2);
                                                continue;
                                            }
                                        } else {
                                            break;
                                        }
                                    } else {
                                        escape = 1;          /* continue lBF4D */
                                        break;
                                    }
                                } else {
                                    escape = 1;          /* continue lBF4D */
                                    break;
                                }
                            } else {
                                drop_z_bound = 0;
                                fx_sbk(drop_z_bound);
                                past_drop_end = 1;
                                fx_stw(drop_ptr, 0);
                                drop_at = (uint16_t)(drop_ptr + 2);
                                continue;
                            }
                        } else {
                            drop_z_bound = 0x01FF;
                            fx_sbk(drop_z_bound);
                            past_drop_end = 1;
                            fx_stw(drop_ptr, 0);
                            drop_at = (uint16_t)(drop_ptr + 2);
                            continue;
                        }
                    } else {
                        fx_stb(drop_at, (uint8_t)(uint16_t)(drop_life_less_one + 2));
                        past_drop_end = ((uint32_t)drop_at + 0x000A > 0xFFFF);
                        drop_at = (uint16_t)(drop_at + 0x000A);
                        continue;
                    }
                }
            } else {
                escape = 2;          /* break lBF4D */
                break;
            }
        }
        if (escape == 1) {
            escape = 0;
            continue;
        }
        if (escape == 2) {
            escape = 0;
            break;
        }
        continue;
    }
    fx_stw(0x01D0, seed);
    fx_unspill(stack);
    return (FxWeatherDropNextResult){1};
}

