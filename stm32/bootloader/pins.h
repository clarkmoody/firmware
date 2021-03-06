/*
 * (c) Copyright 2018 by Coinkite Inc. This file is part of Coldcard <coldcardwallet.com>
 * and is covered by GPLv3 license found in COPYING.
 *
 * pins.h -- everything to do with PIN's and their policies
 *
 */
#pragma once
#include "basics.h"
#include "ae.h"

/* Todo Someday... 
- return a 2x32-byte "ticket" on successful login
- it's encrypted w/ XOR SHA256(pairing secret + powerup seed + salt + index)
- contains successful keynum, actual pin hash, is_duress flag, attempt counter.
- exchange anytime for the secret; checks only counter hasn't changed (and does round-trip to AE)
- pin and secret changes should incr attempt counter
*/

// We hash it like we don't care, but PIN code is expected to be
// just digits, no punctuation, and up to this many char long.
// Using pin+len rather than c-strings. Use zero-length for "blank" or "undefined" pins.
//
#define MAX_PIN_LEN             32

// Number of bytes (per pin) we are keeping secret.
// ATECC508A limitation/feature caps this at weird 72 byte value.
#define AE_SECRET_LEN           72

// For change_flags field: choose one secret and/or one PIN only.
#define CHANGE_WALLET_PIN           0x01
#define CHANGE_DURESS_PIN           0x02
#define CHANGE_BRICKME_PIN          0x04
#define CHANGE_SECRET               0x08
#define CHANGE_DURESS_SECRET        0x10
#define CHANGE_SECONDARY_WALLET_PIN 0x20     // when used from main wallet only
#define CHANGE__MASK                0x3f

// Magic value and/or version number.
#define PA_MAGIC            0x2eaf6311

// For state_flags field: report only covers current wallet (primary vs. secondary)
#define PA_SUCCESSFUL         0x01
#define PA_IS_BLANK           0x02
#define PA_HAS_DURESS         0x04
#define PA_HAS_BRICKME        0x08
#define PA_ZERO_SECRET        0x10

typedef struct {
    uint32_t    magic_value;            // = PA_MAGIC
    int         is_secondary;           // (bool) primary or secondary
    char        pin[MAX_PIN_LEN];       // value being attempted
    int         pin_len;                // valid length of pin
    uint32_t    delay_achieved;         // so far, how much time wasted?
    uint32_t    delay_required;         // how much will be needed?
    uint32_t    num_fails;              // for UI: number of fails PINs
    uint32_t    attempt_target;         // counter number from chip
    uint32_t    state_flags;            // what things have been setup/enabled already
    uint32_t    private_state;          // some internal (encrypted) state
    uint8_t     hmac[32];               // my hmac over above, or zeros
    // remaining fields are return values, or optional args;
    int         change_flags;           // bitmask of what to do
    char        old_pin[MAX_PIN_LEN];   // (optional) old PIN value
    int         old_pin_len;            // (optional) valid length of old_pin, can be zero
    char        new_pin[MAX_PIN_LEN];   // (optional) new PIN value
    int         new_pin_len;            // (optional) valid length of new_pin, can be zero
    uint8_t     secret[AE_SECRET_LEN];  // secret to be changed / return value
    // may grow from here in future versions.
} pinAttempt_t;

#define PIN_ATTEMPT_SIZE        (176+AE_SECRET_LEN)

// Errors codes
enum {
    EPIN_HMAC_FAIL          = -100,
    EPIN_HMAC_REQUIRED      = -101,
    EPIN_BAD_MAGIC          = -102,
    EPIN_RANGE_ERR          = -103,
    EPIN_BAD_REQUEST        = -104,     // bad change flags
    EPIN_I_AM_BRICK         = -105,     // chip has been bricked
    EPIN_AE_FAIL            = -106,     // low-level fails; retry ok
    EPIN_MUST_WAIT          = -107,     // you haven't waited long enough
    EPIN_PIN_REQUIRED       = -108,     // must be non-zero length
    EPIN_WRONG_SUCCESS      = -109,     // success field is not what is needs to be 
    EPIN_OLD_ATTEMPT        = -110,     // tried to recycle older attempt
    EPIN_AUTH_MISMATCH      = -111,     // need pin1 to change duress1, etc.
    EPIN_AUTH_FAIL          = -112,     // pin is wrong
    EPIN_OLD_AUTH_FAIL      = -113,     // existing pin is wrong (during change attempt)
    EPIN_PRIMARY_ONLY       = -114,     // only primary pin can change brickme
};

// Get number of failed attempts on a PIN, since last success. Calculate
// required delay, and setup initial struct for later attempts. Does not
// attempt the PIN or return secrets if right.
int pin_setup_attempt(pinAttempt_t *args);

// Delay for one time unit, and prove it. Doesn't check PIN value itself.
int pin_delay(pinAttempt_t *args);

// Do the PIN check, and return a value. Or fail.
int pin_login_attempt(pinAttempt_t *args);

// Change the PIN and/or secrets (must also know the value, or it must be blank)
int pin_change(pinAttempt_t *args);

// To encourage not keeping the secret in memory, a way to fetch it after already prove you
// know the PIN right.
int pin_fetch_secret(pinAttempt_t *args);

// Record current flash checksum and make green light go on.
int pin_firmware_greenlight(pinAttempt_t *args);

// Return 32 bits of bits which are presistently mapped from pin code; for anti-phishing feature.
int pin_prefix_words(const char *pin_prefix, int prefix_len, uint32_t *result);

// EOF
