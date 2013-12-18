/* Copyright (c) 2013 William Orr <will@worrbase.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "config.h"
#include <glib.h>
#include <stdlib.h>

#include "auth.pb-c.h"

static const gsize ai_encoded_len = 12;
static const guint8 ai_encoded[12]
    = { 0x0a, 0x04, 0x77, 0x69, 0x6c, 0x6c, 0x12, 0x04, 0x74, 0x65, 0x73, 0x74 };
static gchar* ai_username = "will";
static gchar* ai_password = "test";

static const gsize ai_corrupted_encoded_len = 12;
static const guint8 ai_corrupted_encoded[11]
    = { 0x0a, 0x04, 0x77, 0x69, 0x6c, 0x6c, 0x04, 0x74, 0x65, 0x73, 0x74 };

static void test_packing_auth_proto(void) {
	AuthInfo ai = AUTH_INFO__INIT;
	guint8* buf;
	gsize len;

	ai.username = ai_username;
	ai.password = ai_password;

	len = auth_info__get_packed_size(&ai);

	buf = g_slice_alloc0(len);
	auth_info__pack(&ai, buf);

	g_assert(len == ai_encoded_len);

	for (gsize i = 0; i < len; i++) {
		g_assert(buf[i] == ai_encoded[i]);
	}

	g_slice_free1(len, buf);
}

static void test_unpacking_auth_proto(void) {
	AuthInfo* ai = NULL;

	ai = auth_info__unpack(NULL, ai_encoded_len, ai_encoded);

	g_assert(ai != NULL);
	g_assert_cmpstr(ai->username, ==, ai_username);
	g_assert_cmpstr(ai->password, ==, ai_password);

	auth_info__free_unpacked(ai, NULL);
}

#if GLIB_CHECK_VERSION(2, 38, 0)
static void test_unpacking_corrupted_subprocess(void) {
	AuthInfo* ai = NULL;
	ai = auth_info__unpack(NULL, ai_corrupted_encoded_len, ai_corrupted_encoded);
	g_assert(ai == NULL);

	exit(0);
}
#endif

static void test_unpacking_corrupted(void) {
#if GLIB_CHECK_VERSION(2, 38, 0)
	g_test_trap_subprocess("/Library/Protocol/UnpackCorruptedAuthInfo/subprocess",
	                       0, 0);
	g_test_trap_assert_passed();
	g_test_trap_assert_stdout("unsupported tag * at offset *\n");
#else
	AuthInfo* ai = NULL;
	if (g_test_trap_fork(0, G_TEST_TRAP_SILENCE_STDOUT)) {
		ai = auth_info__unpack(NULL, ai_corrupted_encoded_len, ai_corrupted_encoded);
		exit(0);
	}

	g_test_trap_assert_stdout("unsupported tag * at offset *\n");
	g_assert(ai == NULL);
#endif
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/Library/Protocol/PackAuthInfo", test_packing_auth_proto);
	g_test_add_func("/Library/Protocol/UnpackAuthInfo", test_unpacking_auth_proto);
	g_test_add_func("/Library/Protocol/UnpackCorruptedAuthInfo",
	                test_unpacking_corrupted);

#if GLIB_CHECK_VERSION(2, 38, 0)
	g_test_add_func("/Library/Protocol/UnpackCorruptedAuthInfo/subprocess",
	                test_unpacking_corrupted_subprocess);
#endif

	return g_test_run();
}

