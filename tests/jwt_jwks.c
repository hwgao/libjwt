/* Public domain, no copyright. Use at your own risk. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "jwt_tests.h"

#define JWKS_KEY_TEST(__name)				\
START_TEST(test_jwks_##__name)				\
{							\
	SET_OPS();					\
       __jwks_check(#__name ".json", "pem-files/"	\
		#__name ".pem");			\
}							\
END_TEST

static void __jwks_check(const char *json, const char *pem)
{
	JWT_CONFIG_DECLARE(config);
	jwk_set_auto_t *jwk_set = NULL;
	jwk_item_t *item = NULL;
	jwt_auto_t *jwt = NULL;
	char *out = NULL;
	int ret;

        /* Load up the JWKS */
        read_key(json);
	jwk_set = jwks_create(test_data.key);
	free_key();
	ck_assert_ptr_nonnull(jwk_set);

	ck_assert(!jwks_error(jwk_set));

	/* Make sure we have one item */
	item = jwks_item_get(jwk_set, 0);
	ck_assert_ptr_nonnull(item);

	/* If the key is not any provider, we need ops support. */
	if (item->provider != JWT_CRYPTO_OPS_ANY &&
	    !jwt_crypto_ops_supports_jwk()) {
		ck_assert(item->error);
		return;
	}

	if (item->kty != JWK_KEY_TYPE_OCT) {
		read_key(pem);
		ret = strcmp(item->pem, test_data.key);
		free_key();
		ck_assert_int_eq(ret, 0);
	}

	/* Should only be one key in the set */
	item = jwks_item_get(jwk_set, 1);
	ck_assert_ptr_null(item);

	/* Now create a token */
	item = jwks_item_get(jwk_set, 0);
	ck_assert_ptr_nonnull(item);

	if (!item->is_private_key)
		return;

	if (item->alg == JWT_ALG_NONE && item->kty == JWK_KEY_TYPE_RSA) {
		/* "alg" is optional, and it's missing in a few keys */
		config.alg = JWT_ALG_RS256;
	}

	/* Use our JWK */
	config.jw_key = item;
	jwt = jwt_create(&config);
	ck_assert_ptr_nonnull(jwt);

	/* Add some grants */
	ret = jwt_add_grant(jwt, "iss", "files.maclara-llc.com");
        ck_assert_int_eq(ret, 0);

        ret = jwt_add_grant(jwt, "sub", "user0");
        ck_assert_int_eq(ret, 0);

        ret = jwt_add_grant(jwt, "ref", "XXXX-YYYY-ZZZZ-AAAA-CCCC");
        ck_assert_int_eq(ret, 0);

        ret = jwt_add_grant_int(jwt, "iat", TS_CONST);
        ck_assert_int_eq(ret, 0);

	/* Encode it */
        out = jwt_encode_str(jwt);
        ck_assert_ptr_nonnull(out);

	/* Verify it using our JWK */
        __verify_jwk(out, item);

        jwt_free_str(out);
}

JWKS_KEY_TEST(ec_key_prime256v1);
JWKS_KEY_TEST(ec_key_prime256v1_pub);
JWKS_KEY_TEST(ec_key_secp256k1);
JWKS_KEY_TEST(ec_key_secp256k1_pub);
JWKS_KEY_TEST(ec_key_secp384r1);
JWKS_KEY_TEST(ec_key_secp384r1_pub);
JWKS_KEY_TEST(ec_key_secp521r1);
JWKS_KEY_TEST(ec_key_secp521r1_pub);

JWKS_KEY_TEST(eddsa_key_ed25519);
JWKS_KEY_TEST(eddsa_key_ed25519_pub);

JWKS_KEY_TEST(rsa_key_2048);
JWKS_KEY_TEST(rsa_key_2048_pub);
JWKS_KEY_TEST(rsa_key_4096);
JWKS_KEY_TEST(rsa_key_4096_pub);
JWKS_KEY_TEST(rsa_key_8192);
JWKS_KEY_TEST(rsa_key_8192_pub);

JWKS_KEY_TEST(rsa_key_i37_pub);

JWKS_KEY_TEST(rsa_pss_key_2048);
JWKS_KEY_TEST(rsa_pss_key_2048_pub);

JWKS_KEY_TEST(oct_key_256);
JWKS_KEY_TEST(oct_key_384);
JWKS_KEY_TEST(oct_key_512);

START_TEST(test_jwks_keyring_load)
{
	jwk_item_t *item;
	int i;

	SET_OPS_JWK();

	read_key("jwks_keyring.json");

	ck_assert(!jwks_error(g_jwk_set));

	for (i = 0; (item = jwks_item_get(g_jwk_set, i)); i++)
		ck_assert(!item->error);

	ck_assert_int_eq(i, 22);

	ck_assert(jwks_item_free(g_jwk_set, 3));

	free_key();
}
END_TEST

START_TEST(test_jwks_key_op_all_types)
{
	jwk_key_op_t key_ops = JWK_KEY_OP_SIGN | JWK_KEY_OP_VERIFY |
		JWK_KEY_OP_ENCRYPT | JWK_KEY_OP_DECRYPT | JWK_KEY_OP_WRAP |
		JWK_KEY_OP_UNWRAP | JWK_KEY_OP_DERIVE_KEY |
		JWK_KEY_OP_DERIVE_BITS;

	jwk_item_t *item;

	SET_OPS_JWK();

	read_key("jwks_test-1.json");

	item = jwks_item_get(g_jwk_set, 0);
	ck_assert_ptr_nonnull(item);
	ck_assert(!item->error);

	ck_assert_int_eq(item->key_ops, key_ops);

	free_key();
}
END_TEST

START_TEST(test_jwks_key_op_bad_type)
{
	jwk_item_t *item;
	const char *msg = "JWK has an invalid value in key_op";
	const char *kid = "264265c2-4ef0-4751-adbd-9739550afe5b";

	SET_OPS_JWK();

	read_key("jwks_test-2.json");

	item = jwks_item_get(g_jwk_set, 0);
	ck_assert_ptr_nonnull(item);

	/* One item had a bad type (numeric). */
	ck_assert(item->error);
	ck_assert_str_eq(item->error_msg, msg);

	/* Only these ops set. */
	ck_assert_int_eq(item->key_ops,
		JWK_KEY_OP_VERIFY | JWK_KEY_OP_DERIVE_BITS);

	ck_assert_int_eq(item->use, JWK_PUB_KEY_USE_ENC);

	/* Check this key ID. */
	ck_assert_str_eq(item->kid, kid);

	free_key();
}
END_TEST

static Suite *libjwt_suite(const char *title)
{
	Suite *s;
	TCase *tc_core;
	int i = ARRAY_SIZE(jwt_test_ops);

	s = suite_create(title);

	tc_core = tcase_create("jwt_jwks");

	/* Testing individual keys */
	tcase_add_loop_test(tc_core, test_jwks_ec_key_prime256v1, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_ec_key_prime256v1_pub, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_ec_key_secp256k1, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_ec_key_secp256k1_pub, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_ec_key_secp384r1, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_ec_key_secp384r1_pub, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_ec_key_secp521r1, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_ec_key_secp521r1_pub, 0, i);

	tcase_add_loop_test(tc_core, test_jwks_eddsa_key_ed25519, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_eddsa_key_ed25519_pub, 0, i);

	tcase_add_loop_test(tc_core, test_jwks_rsa_key_2048, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_rsa_key_2048_pub, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_rsa_key_4096, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_rsa_key_4096_pub, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_rsa_key_8192, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_rsa_key_8192_pub, 0, i);

	tcase_add_loop_test(tc_core, test_jwks_rsa_key_i37_pub, 0, i);

	tcase_add_loop_test(tc_core, test_jwks_rsa_pss_key_2048, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_rsa_pss_key_2048_pub, 0, i);

	tcase_add_loop_test(tc_core, test_jwks_oct_key_256, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_oct_key_384, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_oct_key_512, 0, i);

	/* Load a whole keyring of all of the above. */
	tcase_add_loop_test(tc_core, test_jwks_keyring_load, 0, i);

	/* Some coverage attempts. */
	tcase_add_loop_test(tc_core, test_jwks_key_op_all_types, 0, i);
	tcase_add_loop_test(tc_core, test_jwks_key_op_bad_type, 0, i);

	tcase_set_timeout(tc_core, 30);

	suite_add_tcase(s, tc_core);

	return s;
}

int main(int argc, char *argv[])
{
	JWT_TEST_MAIN("LibJWT JWKS");
}
