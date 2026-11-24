#include "say.h"
#include "schedule.h"
#include "rle.h"
#include "blob.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(const char *message) {
    fprintf(stderr, "schedule: %s\n", message);
    return 1;
}

static int expect_identity(const char *character, const char *expression) {
    poteto_frame_t frame;
    if (poteto_frame_at(character, expression, 0, &frame) != 0) {
        fprintf(stderr, "schedule: %s %s was rejected\n", character, expression);
        return 1;
    }
    if (strcmp(frame.character, character) != 0 || strcmp(frame.expression, expression) != 0) {
        fprintf(stderr, "schedule: identity mismatch for %s %s\n", character, expression);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    poteto_frame_t first;
    poteto_frame_t wrapped;
    poteto_frame_t step;
    poteto_frame_t from_line;
    FILE *png;
    FILE *blob;
    long blob_size;
    uint8_t *bytes;
    uint16_t *pixels;
    size_t decoded;
    size_t i;
    int nonzero;
    const char *root;
    char png_path[512];
    char bin_path[512];
    int step_ms;

    root = argc > 1 ? argv[1] : ".";
    if (poteto_frame_at("poteto", "idle", 0, &first) != 0) {
        return fail("idle lookup failed");
    }
    if (strcmp(first.path, "poteto/idle/f00.png") != 0 || first.frame_index != 0 || first.frame_count < 1) {
        return fail("poteto idle did not resolve to its first packed frame");
    }
    snprintf(png_path, sizeof(png_path), "%s/assets/frames/%s", root, first.path);
    png = fopen(png_path, "rb");
    if (png == NULL) {
        fprintf(stderr, "schedule: missing %s\n", png_path);
        return 1;
    }
    fclose(png);

    step_ms = 1000 / first.fps;
    if (poteto_frame_at("poteto", "idle", step_ms * first.frame_count, &wrapped) != 0) {
        return fail("wrap lookup failed");
    }
    if (strcmp(first.path, wrapped.path) != 0 || first.frame_index != wrapped.frame_index) {
        return fail("loop did not return to the first frame");
    }
    if (first.frame_count > 1) {
        if (poteto_frame_at("poteto", "idle", step_ms, &step) != 0 || step.frame_index != 1) {
            return fail("second frame was not selected");
        }
    }

    if (poteto_frame_at("nope", "idle", 0, &first) == 0) {
        return fail("unknown character was accepted");
    }
    if (poteto_apply_line("SEL v1 sunny idle", &from_line) != 0) {
        return fail("SEL line was rejected");
    }
    if (strcmp(from_line.character, "sunny") != 0 || strcmp(from_line.path, "sunny/idle/f00.png") != 0) {
        return fail("SEL line did not select sunny idle");
    }
    if (expect_identity("poteto", "speaking") || expect_identity("poteto", "listening") ||
        expect_identity("poteto", "thinking") || expect_identity("sunny", "listening") ||
        expect_identity("sunny", "thinking")) {
        return 1;
    }
    if (poteto_frame_at("poteto", "speaking", 0, &from_line) != 0 || from_line.frame_count != 4 ||
        strcmp(from_line.path, "poteto/speaking/f00.png") != 0) {
        return fail("poteto speaking is not the packed 4-frame loop");
    }
    if (poteto_frame_at("poteto", "speaking", (1000 / 6) * 2, &from_line) != 0 ||
        from_line.frame_index != 2 || strcmp(from_line.path, "poteto/speaking/f02.png") != 0) {
        return fail("poteto speaking did not advance to f02");
    }
    if (poteto_frame_at("sunny", "listening", 0, &from_line) != 0 ||
        strcmp(from_line.path, "sunny/listening/f00.png") != 0) {
        return fail("sunny listening frame is not packed");
    }
    if (poteto_frame_at("sunny", "thinking", 0, &from_line) != 0 ||
        strcmp(from_line.path, "sunny/thinking/f00.png") != 0) {
        return fail("sunny thinking frame is not packed");
    }
    if (poteto_frame_at("sunny", "speaking", (1000 / 6) * 3, &from_line) != 0 ||
        from_line.frame_index != 3 || strcmp(from_line.path, "sunny/speaking/f03.png") != 0) {
        return fail("sunny speaking did not advance to f03");
    }
    if (poteto_frame_at("poteto", "listening", 0, &from_line) != 0 || from_line.frame_count != 2 ||
        strcmp(from_line.path, "poteto/listening/f00.png") != 0) {
        return fail("poteto listening frame is not packed");
    }
    if (poteto_frame_at("poteto", "listening", 1000 / 15, &from_line) != 0 || from_line.frame_index != 1 ||
        strcmp(from_line.path, "poteto/listening/f01.png") != 0) {
        return fail("poteto listening did not advance");
    }
    if (poteto_frame_at("poteto", "thinking", 0, &from_line) != 0 || from_line.frame_count != 2 ||
        strcmp(from_line.path, "poteto/thinking/f00.png") != 0) {
        return fail("poteto thinking frame is not packed");
    }
    if (poteto_frame_at("poteto", "thinking", 1000 / 12, &from_line) != 0 || from_line.frame_index != 1 ||
        strcmp(from_line.path, "poteto/thinking/f01.png") != 0) {
        return fail("poteto thinking did not advance");
    }
    if (poteto_frame_count("poteto", "smile") != 0) {
        return fail("poteto smile is still packed");
    }
    if (poteto_frame_count("poteto", "emphasis") != 0) {
        return fail("poteto emphasis returned to the factory blob");
    }
    if (poteto_apply_line("SEL v1 poteto emphasis", &from_line) == 0) {
        return fail("poteto emphasis was accepted");
    }

    snprintf(bin_path, sizeof(bin_path), "%s/firmware/main/frames.bin", root);
    blob = fopen(bin_path, "rb");
    if (blob == NULL) {
        return fail("frames.bin is missing");
    }
    if (fseek(blob, 0, SEEK_END) != 0) {
        fclose(blob);
        return fail("frames.bin seek failed");
    }
    blob_size = ftell(blob);
    if (blob_size < 16) {
        fclose(blob);
        return fail("frames.bin is too small");
    }
    fseek(blob, 0, SEEK_SET);
    bytes = (uint8_t *)malloc((size_t)blob_size);
    if (bytes == NULL || fread(bytes, 1, (size_t)blob_size, blob) != (size_t)blob_size) {
        fclose(blob);
        free(bytes);
        return fail("frames.bin read failed");
    }
    fclose(blob);
    if (poteto_frame_at("poteto", "idle", 0, &first) != 0) {
        free(bytes);
        return fail("idle lookup failed before decode");
    }
    {
        const uint8_t *payload = NULL;
        size_t payload_len = 0;
        if (poteto_blob_find(bytes, (size_t)blob_size, "poteto", "idle", first.frame_index, &payload, &payload_len) != 0) {
            free(bytes);
            return fail("packed blob has no poteto idle frame");
        }
        pixels = (uint16_t *)calloc((size_t)POTETO_CANVAS * POTETO_CANVAS, sizeof(uint16_t));
        if (pixels == NULL) {
            free(bytes);
            return fail("pixel buffer alloc failed");
        }
        decoded = poteto_rle_decode(payload, payload_len, pixels, (size_t)POTETO_CANVAS * POTETO_CANVAS);
        if (decoded != (size_t)POTETO_CANVAS * POTETO_CANVAS) {
            free(pixels);
            free(bytes);
            return fail("rle decode did not fill the canvas");
        }
        if (pixels[0] != 0) {
            free(pixels);
            free(bytes);
            return fail("top-left pixel is not black");
        }
        nonzero = 0;
        for (i = 0; i < decoded; i++) {
            if (pixels[i] != 0) {
                nonzero = 1;
                break;
            }
        }
        free(pixels);
        free(bytes);
        if (!nonzero) {
            return fail("decoded frame is empty");
        }
    }
    {
        char phase[POTETO_SAY_PHASE_MAX];
        char text[POTETO_SAY_TEXT_MAX];
        if (poteto_apply_say("TXT v1 thinking\n", phase, sizeof(phase), text, sizeof(text)) != 0 ||
            strcmp(phase, "thinking") != 0 || strcmp(text, "...") != 0) {
            return fail("thinking line was parsed wrong");
        }
        if (poteto_apply_say("TXT v1 \xEC\x95\x88\xEB\x85\x95\n", phase, sizeof(phase), text, sizeof(text)) != 0 ||
            strcmp(phase, "answer") != 0 || strcmp(text, "\xEC\x95\x88\xEB\x85\x95") != 0) {
            return fail("answer text was parsed wrong");
        }
        if (poteto_apply_say("SEL v1 poteto idle\n", phase, sizeof(phase), text, sizeof(text)) == 0) {
            return fail("SEL line was treated as text");
        }
        if (poteto_apply_line("TXT v1 hello", &from_line) == 0) {
            return fail("TXT line was treated as a selection");
        }
    }
    printf("schedule ok\n");
    return 0;
}
