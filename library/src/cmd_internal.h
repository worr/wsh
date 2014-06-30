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
/** @cond */
#ifdef BUILD_TESTS

/**
 * @brief Checks stdout for new output to save in our wsh_cmd_res_t
 *
 * @param[in] out The GIOChannel to check
 * @param[in] cond The GIOCondition that triggered the call
 * @param[out] data Transient main loop data
 *
 * @returns FALSE if event source should be removed
 */
gboolean check_stdout(GIOChannel* out, GIOCondition cond,
                          struct cmd_data* data);

/**
 * @brief Checks stderr for new output to save in our wsh_cmd_res_t
 *
 * @param[in] err The GIOChannel to check
 * @param[in] cond The GIOCondition that triggered the call
 * @param[out] data Transient main loop data
 *
 * @returns FALSE if event source should be removed
 */
gboolean check_stderr(GIOChannel* err, GIOCondition cond,
                          struct cmd_data* data);

/**
 * @brief Writes stdin to the command
 *
 * @param[in] in The GIOChannel to write to
 * @param[in] cond The GIOCondition that triggered the call
 * @param[out] data Transient main loop data
 *
 * @returns FALSE if event source should be removed
 */
gboolean write_stdin(GIOChannel* in, GIOCondition cond,
                         struct cmd_data* data);

struct test_cmd_data {
	GMainLoop* loop;
	wsh_cmd_req_t* req;
	wsh_cmd_res_t* res;
	gboolean cmd_exited;
	gboolean out_closed;
	gboolean err_closed;
};

#endif
/** @endcond */

