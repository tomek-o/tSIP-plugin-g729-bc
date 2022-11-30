/**
 * @file g729.c  G.729 audio codec module
 */
#include <stdlib.h>
#include <string.h>
#include <re.h>
#include <baresip.h>
#include <module.h>
#include <assert.h>

#include "bcg729/encoder.h"
#include "bcg729/decoder.h"

extern mem_alloc_cb mem_alloc_fn;
extern mem_deref_cb mem_deref_fn;
extern aucodec_register_cb aucodec_register_fn;
extern aucodec_unregister_cb aucodec_unregister_fn;

struct auenc_state {
	bcg729EncoderChannelContextStruct *enc;
};

static void auenc_state_destructor(void *arg) {
    struct auenc_state *s = arg;

    if (s->enc) {
		closeBcg729EncoderChannel(s->enc);
		s->enc = NULL;
    }
}

struct audec_state {
	bcg729DecoderChannelContextStruct *dec;
};

static void audec_state_destructor(void *arg) {
    struct audec_state *s = arg;

    if (s->dec) {
		closeBcg729DecoderChannel(s->dec);
		s->dec = NULL;
    }
}


static int encode_update(struct auenc_state **aesp,
                         const struct aucodec *ac,
                         struct auenc_param *prm, const char *fmtp) {
    struct auenc_state *st;
    int err = 0;
    (void)prm;
    (void)fmtp;

    if (!aesp || !ac)
        return EINVAL;

    if (*aesp)
        return 0;

    st = mem_alloc_fn(sizeof(*st), auenc_state_destructor);
    if (!st)
        return ENOMEM;
     
    st->enc = initBcg729EncoderChannel(0); 
    if (!st->enc)
        return ENOMEM;

//out:
    if (err)
        mem_deref_fn(st);
    else
        *aesp = st;

    return err;
}


static int decode_update(struct audec_state **adsp,
                         const struct aucodec *ac, const char *fmtp) {
    struct audec_state *st;
    int err = 0;
    (void)fmtp;

    if (!adsp || !ac)
        return EINVAL;

    if (*adsp)
        return 0;

    st = mem_alloc_fn(sizeof(*st), audec_state_destructor);
    if (!st)
        return ENOMEM;

    st->dec = initBcg729DecoderChannel();
    if (!st->dec)
        return ENOMEM;

//out:
    if (err)
        mem_deref_fn(st);
    else
        *adsp = st;

    return err;
}

static int encode(struct auenc_state *st, uint8_t *buf, size_t *len,
                  const int16_t *sampv, size_t sampc) {
    assert(sampc % 80 == 0);
    int total_len = 0;
    while (sampc >= 80) {
		uint8_t frameSize = 0;
		bcg729Encoder(st->enc, sampv, buf, &frameSize);		
        sampv += 80;
        sampc -= 80;
        buf += frameSize;
        total_len += frameSize;
    }
    *len = total_len;
    return 0;
}

static int decode(struct audec_state *st, int16_t *sampv, size_t *sampc,
                  const uint8_t *buf, size_t len) {
    if (!st || !sampv || !buf)
        return EINVAL;
    assert(len % 10 == 0);
    int total_sampc = 0;
    while (len >= 10) {		
		uint8_t isSID = 0;
        bcg729Decoder(st->dec, buf, 10, 0, isSID, 0, sampv);			
        buf += 10;
        len -= 10;
        sampv += 80;
        total_sampc += 80;
    }
    *sampc = total_sampc;

    return 0;
}


static struct aucodec g729 = {
    LE_INIT, "18", "G729", 8000, 1, NULL,
    encode_update, encode,
    decode_update, decode, NULL,
    NULL, NULL
};


static int module_init(void) {
    aucodec_register_fn(&g729);
    return 0;
}


static int module_close(void) {
    aucodec_unregister_fn(&g729);
    return 0;
}


/** Module exports */
EXPORT_SYM const struct mod_export DECL_EXPORTS(g729) = {
    "g729",
    "codec",
    module_init,
    module_close
};
