#include "channel.h"

#include <stdio.h>
#include <string.h>

static int fail(const char *message) {
    fprintf(stderr, "channel: %s\n", message);
    return 1;
}

int main(void) {
    const char *json =
        "note body\":\"stolen\" thread_id\":\"msg_2787a1c295c8\"\n"
        "{\n"
        "  \"messages\": [\n"
        "    {\n"
        "      \"from_handle\": \"orca_session_id:old\",\n"
        "      \"body\": \"old answer\",\n"
        "      \"thread_id\": \"msg_old\",\n"
        "      \"payload\": \"{\\\"requestId\\\":\\\"msg_old\\\"}\"\n"
        "    },\n"
        "    {\n"
        "      \"thread_id\": \"msg_2787a1c295c8\",\n"
        "      \"payload\": \"{\\\"requestId\\\":\\\"msg_2787a1c295c8\\\",\\\"botAnswer\\\":{}}\",\n"
        "      \"from_handle\": \"orca_session_id:b4c49b90-88f3-4c72-b0a5-9e9d1a4c1957\",\n"
        "      \"body\": \"391\"\n"
        "    }\n"
        "  ]\n"
        "}\n";
    char answer[32];
    char from[80];
    if (poteto_channel_match(json, "msg_2787a1c295c8", answer, sizeof(answer), from, sizeof(from)) != 0) {
        return fail("live-shaped row was not the final answer");
    }
    if (strcmp(answer, "391") != 0) {
        return fail("answer was not 391");
    }
    if (strcmp(from, "orca_session_id:b4c49b90-88f3-4c72-b0a5-9e9d1a4c1957") != 0) {
        return fail("from_handle mismatch");
    }
    if (poteto_channel_match(json, "msg_missing", answer, sizeof(answer), from, sizeof(from)) == 0) {
        return fail("missing request looked answered");
    }
    if (poteto_channel_match(json, "msg_old", answer, sizeof(answer), from, sizeof(from)) != 0 ||
        strcmp(answer, "old answer") != 0) {
        return fail("older row should still match only its own id");
    }
    {
        const char *twice =
            "{\"messages\":["
            "{\"thread_id\":\"msg_2787a1c295c8\",\"body\":\"391\",\"from_handle\":\"orca_session_id:a\","
            "\"payload\":\"{\\\"requestId\\\":\\\"msg_2787a1c295c8\\\"}\"},"
            "{\"thread_id\":\"msg_2787a1c295c8\",\"body\":\"392\",\"from_handle\":\"orca_session_id:b\","
            "\"payload\":\"{\\\"requestId\\\":\\\"msg_2787a1c295c8\\\"}\"}"
            "]}";
        if (poteto_channel_match(twice, "msg_2787a1c295c8", answer, sizeof(answer), from, sizeof(from)) == 0) {
            return fail("two objects with the same request id were accepted");
        }
    }
    printf("match ok\n");
    return 0;
}
