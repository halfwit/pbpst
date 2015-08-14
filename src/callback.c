#include "main.h"
#include "callback.h"

/* adapted from pacman */
static void
fill_progress (const uint8_t bar_percent, const uint8_t proglen) {

    /* 8 = 1 space + 1 [ + 1 ] + 5 for percent */
    const uint8_t hashlen = proglen > 8 ? proglen - 8 : 0,
                  hash    = bar_percent * hashlen / 100;

    if ( hashlen > 0 ) {
        fputs(" [", stderr);
        for ( uint8_t i = hashlen; i; --i ) {
            fputc(i > (hashlen - hash) ? '#' : '-', stderr);
        } fputc(']', stderr);
    }

    /* print display percent after progress bar */
    if ( proglen >= 5 ) { fprintf(stderr, " %3" PRIu8 "%%", bar_percent); }

    fputc(bar_percent == 100 ? '\n' : '\r', stderr);
    fflush(stderr);
}

signed
pb_progress_cb (void * client,
                curl_off_t dltotal, curl_off_t dlnow,
                curl_off_t ultotal, curl_off_t ulnow) {

    static curl_off_t last_progress;
    curl_off_t progress = ultotal ? ulnow * 100 / ultotal : 0;

    if ( progress == last_progress ) { return 0; }

    fill_progress((uint8_t )progress, 80);
    last_progress = progress;
    return 0;
}

size_t
pb_write_cb (char * ptr, size_t size, size_t nmemb, void * userdata) {

    if ( !ptr || !userdata ) { return 0; }

    size_t rsize = size * nmemb;
    *(ptr + rsize) = '\0';

    json_t * json = json_loads(ptr, 0, NULL);
    if ( !json ) { return 0; }

    json_t * value;
    const char * key;
    json_object_foreach(json, key, value) {
        printf("%s: %s\n", key, json_string_value(value));
    }

    json_t * pastes = json_object_get(mem_db, "pastes");
    if ( !pastes ) { return rsize; }

    json_t * prov_pastes = json_object_get(pastes, state.provider);
    if ( !prov_pastes ) {
        json_t * prov_obj = json_pack("{s:{}}", state.provider);
        json_object_update(pastes, prov_obj);
        json_decref(prov_obj);
        prov_pastes = json_object_get(pastes, state.provider);
    }

    json_t * uuid_j  = json_object_get(json, "uuid"),
           * lid_j   = json_object_get(json, "long"),
           * label_j = json_object_get(json, "label");

    if ( (!uuid_j && !state.uuid) || !lid_j ) { return rsize; }

    const char * uuid  = uuid_j ? json_string_value(uuid_j) : state.uuid,
               * lid   = json_string_value(lid_j),
               * label = json_string_value(label_j);

    json_t * new_paste = json_pack(label_j ? "{s:s,s:s}" : "{s:s,s:n}",
                                   "long", lid, "label", label);

    if ( json_object_set(prov_pastes, uuid, new_paste) == -1 ) {
        return rsize;
    }

    printf("%s%s\n", state.provider, state.priv ? lid : lid + 24);
    json_decref(uuid_j); json_decref(lid_j); json_decref(label_j);
    json_decref(prov_pastes); json_decref(pastes); json_decref(new_paste);
    json_decref(json); return rsize;
}

// vim: set ts=4 sw=4 et:
