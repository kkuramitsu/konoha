/****************************************************************************
 * Copyright (c) 2012, the Konoha project authors. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

/* ************************************************************************ */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */

static inline int lpos(tenv_t *tenv, const char *s)
{
	return (tenv->bol == NULL) ? -1 : s - tenv->bol;
}

static int parseINDENT(CTX, struct _kToken *tk, tenv_t *tenv, int pos, kFunc *thunk)
{
	int ch, c = 0;
	while((ch = tenv->source[pos++]) != 0) {
		if(ch == '\t') { c += tenv->indent_tab; }
		else if(ch == ' ') { c += 1; }
		break;
	}
	if(IS_NOTNULL(tk)) {
		tk->tt = TK_INDENT;
		tk->lpos = 0;
	}
	return pos-1;
}

static int parseNL(CTX, struct _kToken *tk, tenv_t *tenv, int pos, kFunc *thunk)
{
	tenv->uline += 1;
	tenv->bol = tenv->source + pos + 1;
	return parseINDENT(_ctx, tk, tenv, pos+1, thunk);
}

static int parseNUM(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start, kFunc *thunk)
{
	int ch, pos = tok_start, dot = 0;
	const char *ts = tenv->source;
	while((ch = ts[pos++]) != 0) {
		if(ch == '_') continue; // nothing
		if(ch == '.') {
			if(!isdigit(ts[pos])) {
				break;
			}
			dot++;
			continue;
		}
		if((ch == 'e' || ch == 'E') && (ts[pos] == '+' || ts[pos] =='-')) {
			pos++;
			continue;
		}
		if(!isalnum(ch)) break;
	}
	if(IS_NOTNULL(tk)) {
		KSETv(tk->text, new_kString(ts + tok_start, (pos-1)-tok_start, SPOL_ASCII));
		tk->tt = (dot == 0) ? TK_INT : TK_FLOAT;
	}
	return pos - 1;  // next
}

static int parseSYMBOL(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start, kFunc *thunk)
{
	int ch, pos = tok_start;
	const char *ts = tenv->source;
	while((ch = ts[pos++]) != 0) {
		if(ch == '_' || isalnum(ch)) continue; // nothing
		break;
	}
	if(IS_NOTNULL(tk)) {
		KSETv(tk->text, new_kString(ts + tok_start, (pos-1)-tok_start, SPOL_ASCII));
		tk->tt = TK_SYMBOL;
	}
	return pos - 1;  // next
}

static int parseUSYMBOL(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start, kFunc *thunk)
{
	int ch, pos = tok_start;
	const char *ts = tenv->source;
	while((ch = ts[pos++]) != 0) {
		if(ch == '_' || isalnum(ch)) continue; // nothing
		break;
	}
	if(IS_NOTNULL(tk)) {
		KSETv(tk->text, new_kString(ts + tok_start, (pos-1)-tok_start, SPOL_ASCII));
		tk->tt = TK_USYMBOL;
	}
	return pos - 1;  // next
}

static int parseMSYMBOL(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start, kFunc *thunk)
{
	int ch, pos = tok_start;
	const char *ts = tenv->source;
	while((ch = ts[pos++]) != 0) {
		if(!(ch < 0)) break;
	}
	if(IS_NOTNULL(tk)) {
		KSETv(tk->text, new_kString(ts + tok_start, (pos-1)-tok_start, SPOL_UTF8));
		tk->tt = TK_MSYMBOL;
	}
	return pos - 1;  // next
}

static int parseOP1(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start, kFunc *thunk)
{
	if(IS_NOTNULL(tk)) {
		const char *s = tenv->source + tok_start;
		KSETv(tk->text, new_kString(s, 1, SPOL_ASCII|SPOL_POOL));
		tk->tt = TK_OPERATOR;
		tk->topch = s[0];
	}
	return tok_start+1;
}

static int parseOP(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start, kFunc *thunk)
{
	int ch, pos = tok_start;
	while((ch = tenv->source[pos++]) != 0) {
		if(isalnum(ch)) break;
		switch(ch) {
			case '<': case '>': case '@': case '$': case '#':
			case '+': case '-': case '*': case '%': case '/':
			case '=': case '&': case '?': case ':': case '.':
			case '^': case '!': case '~': case '|':
			continue;
		}
		break;
	}
	if(IS_NOTNULL(tk)) {
		const char *s = tenv->source + tok_start;
		KSETv(tk->text, new_kString(s, (pos-1)-tok_start, SPOL_ASCII|SPOL_POOL));
		tk->tt = TK_OPERATOR;
		if(S_size(tk->text) == 1) {
			tk->topch = S_text(tk->text)[0];
		}
	}
	return pos-1;
}

static int parseLINE(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start, kFunc *thunk)
{
	int ch, pos = tok_start;
	while((ch = tenv->source[pos++]) != 0) {
		if(ch == '\n') break;
	}
	return pos-1;/*EOF*/
}

static int parseCOMMENT(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start, kFunc *thunk)
{
	int ch, prev = 0, level = 1, pos = tok_start + 2;
	/*@#nnnn is line number */
	if(tenv->source[pos] == '@' && tenv->source[pos+1] == '#' && isdigit(tenv->source[pos+2])) {
		tenv->uline >>= (sizeof(kshort_t)*8);
		tenv->uline = (tenv->uline<<(sizeof(kshort_t)*8))  | (kshort_t)strtoll(tenv->source + pos + 2, NULL, 10);
	}
	while((ch = tenv->source[pos++]) != 0) {
		if(ch == '\n') {
			tenv->uline += 1;
		}
		if(prev == '*' && ch == '/') {
			level--;
			if(level == 0) return pos;
		}else if(prev == '/' && ch == '*') {
			level++;
		}
		prev = ch;
	}
	if(IS_NOTNULL(tk)) {
		Token_pERR(_ctx, tk, "must close with */");
	}
	return pos-1;/*EOF*/
}

static int parseSLASH(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start, kFunc *thunk)
{
	const char *ts = tenv->source + tok_start;
	if(ts[1] == '/') {
		return parseLINE(_ctx, tk, tenv, tok_start, thunk);
	}
	if(ts[1] == '*') {
		return parseCOMMENT(_ctx, tk, tenv, tok_start, thunk);
	}
	return parseOP(_ctx, tk, tenv, tok_start, thunk);
}

static int parseDQUOTE(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start, kFunc *thunk)
{
	kwb_t wb;
	kwb_init(&(_ctx->stack->cwb), &wb);
	int ch, prev = '"', pos = tok_start + 1, next;
	while((ch = tenv->source[pos++]) != 0) {
		if(ch == '\n') {
			break;
		}
		if(ch == '"' && prev != '\\') {
			if(IS_NOTNULL(tk)) {
				size_t length = kwb_bytesize(&wb);
				KSETv(tk->text, new_kString(kwb_top(&wb, 1), length, 0));
				tk->tt = TK_TEXT;
			}
			kwb_free(&wb);
			return pos;
		}
		if(ch == '\\' && (next = tenv->source[pos]) != 0) {
			switch (next) {
			case 'n': ch = '\n'; pos++; break;
			case 't': ch = '\t'; pos++; break;
			case 'r': ch = '\r'; pos++; break;
			}
		}
		prev = ch;
		kwb_putc(&wb, ch);
	}
	if(IS_NOTNULL(tk)) {
		Token_pERR(_ctx, tk, "must close with \"");
	}
	kwb_free(&wb);
	return pos-1;
}

static int parseSKIP(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start, kFunc *thunk)
{
	return tok_start+1;
}

static int parseUNDEF(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start, kFunc *thunk)
{
	if(IS_NOTNULL(tk)) {
		Token_pERR(_ctx, tk, "undefined token character: %c", tenv->source[tok_start]);
	}
	while(tenv->source[++tok_start] != 0);
	return tok_start;
}

static int parseBLOCK(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start, kFunc *thunk);

static const Ftokenizer MiniKonohaTokenMatrix[] = {
#define _NULL      0
	parseSKIP,
#define _UNDEF     1
	parseSKIP,
#define _DIGIT     2
	parseNUM,
#define _UALPHA    3
	parseUSYMBOL,
#define _LALPHA    4
	parseSYMBOL,
#define _MULTI     5
	parseMSYMBOL,
#define _NL        6
	parseNL,
#define _TAB       7
	parseSKIP,
#define _SP_       8
	parseSKIP,
#define _LPAR      9
	parseOP1,
#define _RPAR      10
	parseOP1,
#define _LSQ       11
	parseOP1,
#define _RSQ       12
	parseOP1,
#define _LBR       13
	parseBLOCK,
#define _RBR       14
	parseOP1,
#define _LT        15
	parseOP,
#define _GT        16
	parseOP,
#define _QUOTE     17
	parseUNDEF,
#define _DQUOTE    18
	parseDQUOTE,
#define _BKQUOTE   19
	parseUNDEF,
#define _OKIDOKI   20
	parseOP,
#define _SHARP     21
	parseOP,
#define _DOLLAR    22
	parseOP,
#define _PER       23
	parseOP,
#define _AND       24
	parseOP,
#define _STAR      25
	parseOP,
#define _PLUS      26
	parseOP,
#define _COMMA     27
	parseOP1,
#define _MINUS     28
	parseOP,
#define _DOT       29
	parseOP,
#define _SLASH     30
	parseSLASH,
#define _COLON     31
	parseOP,
#define _SEMICOLON 32
	parseOP1,
#define _EQ        33
	parseOP,
#define _QUESTION  34
	parseOP,
#define _AT_       35
	parseOP1,
#define _VAR       36
	parseOP,
#define _CHILDER   37
	parseOP,
#define _BKSLASH   38
	parseUNDEF,
#define _HAT       39
	parseOP,
#define _UNDER     40
	parseSYMBOL,
#define KCHAR_MAX  41
};

static const char cMatrix[128] = {
	0/*nul*/, 1/*soh*/, 1/*stx*/, 1/*etx*/, 1/*eot*/, 1/*enq*/, 1/*ack*/, 1/*bel*/,
	1/*bs*/,  _TAB/*ht*/, _NL/*nl*/, 1/*vt*/, 1/*np*/, 1/*cr*/, 1/*so*/, 1/*si*/,
	/*	020 dle  021 dc1  022 dc2  023 dc3  024 dc4  025 nak  026 syn  027 etb*/
	1, 1, 1, 1,     1, 1, 1, 1,
	/*	030 can  031 em   032 sub  033 esc  034 fs   035 gs   036 rs   037 us*/
	1, 1, 1, 1,     1, 1, 1, 1,
	/*040 sp   041  !   042  "   043  #   044  $   045  %   046  &   047  '*/
	_SP_, _OKIDOKI, _DQUOTE, _SHARP, _DOLLAR, _PER, _AND, _QUOTE,
	/*050  (   051  )   052  *   053  +   054  ,   055  -   056  .   057  /*/
	_LPAR, _RPAR, _STAR, _PLUS, _COMMA, _MINUS, _DOT, _SLASH,
	/*060  0   061  1   062  2   063  3   064  4   065  5   066  6   067  7 */
	_DIGIT, _DIGIT, _DIGIT, _DIGIT,  _DIGIT, _DIGIT, _DIGIT, _DIGIT,
	/*	070  8   071  9   072  :   073  ;   074  <   075  =   076  >   077  ? */
	_DIGIT, _DIGIT, _COLON, _SEMICOLON, _LT, _EQ, _GT, _QUESTION,
	/*100  @   101  A   102  B   103  C   104  D   105  E   106  F   107  G */
	_AT_, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA,
	/*110  H   111  I   112  J   113  K   114  L   115  M   116  N   117  O */
	_UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA,
	/*120  P   121  Q   122  R   123  S   124  T   125  U   126  V   127  W*/
	_UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA,
	/*130  X   131  Y   132  Z   133  [   134  \   135  ]   136  ^   137  _*/
	_UALPHA, _UALPHA, _UALPHA, _LSQ, _BKSLASH, _RSQ, _HAT, _UNDER,
	/*140  `   141  a   142  b   143  c   144  d   145  e   146  f   147  g*/
	_BKQUOTE, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA,
	/*150  h   151  i   152  j   153  k   154  l   155  m   156  n   157  o*/
	_LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA,
	/*160  p   161  q   162  r   163  s   164  t   165  u   166  v   167  w*/
	_LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA,
	/*170  x   171  y   172  z   173  {   174  |   175  }   176  ~   177 del*/
	_LALPHA, _LALPHA, _LALPHA, _LBR, _VAR, _RBR, _CHILDER, 1,
};

static int kchar(const char *t, int pos)
{
	int ch = t[pos];
	return (ch < 0) ? _MULTI : cMatrix[ch];
}

static int parseBLOCK(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start, kFunc *thunk)
{
	int ch, level = 1, pos = tok_start + 1;
	const Ftokenizer *fmat = tenv->fmat;
	tk->lpos += 1;
	while((ch = kchar(tenv->source, pos)) != 0) {
		if(ch == _RBR/*}*/) {
			level--;
			if(level == 0) {
				if(IS_NOTNULL(tk)) {
					KSETv(tk->text, new_kString(tenv->source + tok_start + 1, ((pos-2)-(tok_start)+1), 0));
					tk->tt = TK_CODE;
				}
				return pos + 1;
			}
			pos++;
		}
		else if(ch == _LBR/*'{'*/) {
			level++; pos++;
		}
		else {
			pos = fmat[ch](_ctx, (struct _kToken*)K_NULLTOKEN, tenv, pos, NULL);
		}
	}
	if(IS_NOTNULL(tk)) {
		Token_pERR(_ctx, tk, "must close with }");
	}
	return pos-1;
}

static void tokenize(CTX, tenv_t *tenv)
{
	int ch, pos = 0;
	const Ftokenizer *fmat = tenv->fmat;
	struct _kToken *tk = new_W(Token, 0);
	assert(tk->tt == 0);
	tk->uline = tenv->uline;
	tk->lpos  = lpos(tenv, tenv->source);
	pos = parseINDENT(_ctx, tk, tenv, pos, NULL);
	while((ch = kchar(tenv->source, pos)) != 0) {
		if(tk->tt != 0) {
			kArray_add(tenv->list, tk);
			tk = new_W(Token, 0);
			tk->uline = tenv->uline;
			tk->lpos  = lpos(tenv, (tenv->source + pos));
		}
		int pos2 = fmat[ch](_ctx, tk, tenv, pos, NULL);
		assert(pos2 > pos);
		pos = pos2;
	}
	if(tk->tt != 0) {
		kArray_add(tenv->list, tk);
	}
}

static const Ftokenizer *KonohaSpace_tokenizerMatrix(CTX, kKonohaSpace *ks)
{
	if(ks->fmat == NULL) {
		DBG_ASSERT(KCHAR_MAX * sizeof(Ftokenizer) == sizeof(MiniKonohaTokenMatrix));
		Ftokenizer *fmat = (Ftokenizer*)KMALLOC(sizeof(MiniKonohaTokenMatrix));
		if(ks->parentNULL != NULL && ks->parentNULL->fmat != NULL) {
			memcpy(fmat, ks->parentNULL->fmat, sizeof(MiniKonohaTokenMatrix));
		}
		else {
			memcpy(fmat, MiniKonohaTokenMatrix, sizeof(MiniKonohaTokenMatrix));
		}
		((struct _kKonohaSpace*)ks)->fmat = (const Ftokenizer*)fmat;
	}
	return ks->fmat;
}

static void KonohaSpace_setTokenizer(CTX, kKonohaSpace *ks, int ch, Ftokenizer f, kFunc *fo)
{
	int kchar = (ch < 0) ? _MULTI : cMatrix[ch];
	Ftokenizer *fmat = (Ftokenizer*)KonohaSpace_tokenizerMatrix(_ctx, ks);
	fmat[kchar] = f;
}

static void KonohaSpace_tokenize(CTX, kKonohaSpace *ks, const char *source, kline_t uline, kArray *a)
{
	size_t i, pos = kArray_size(a);
	tenv_t tenv = {
		.source = source,
		.uline  = uline,
		.list   = a,
		.bol    = source,
		.indent_tab = 4,
		.fmat   = ks == NULL ? MiniKonohaTokenMatrix : KonohaSpace_tokenizerMatrix(_ctx, ks),
	};
	tokenize(_ctx, &tenv);
	if(uline == 0) {
		for(i = pos; i < kArray_size(a); i++) {
			a->Wtoks[i]->uline = 0;
		}
	}
}

// --------------------------------------------------------------------------

static int findTopCh(CTX, kArray *tls, int s, int e, ktoken_t tt, int closech)
{
	int i;
	for(i = s; i < e; i++) {
		kToken *tk = tls->toks[i];
		if(tk->tt == tt && S_text(tk->text)[0] == closech) return i;
	}
	DBG_ASSERT(i != e);  // Must not happen
	return e;
}

static kbool_t makeSyntaxRule(CTX, kArray *tls, int s, int e, kArray *adst);

static kbool_t checkNestedSyntax(CTX, kArray *tls, int *s, int e, ktoken_t tt, int opench, int closech)
{
	int i = *s;
	struct _kToken *tk = tls->Wtoks[i];
	const char *t = S_text(tk->text);
	if(t[0] == opench && t[1] == 0) {
		int ne = findTopCh(_ctx, tls, i+1, e, tk->tt, closech);
		tk->tt = tt; tk->kw = tt;
		KSETv(tk->sub, new_(TokenArray, 0));
		tk->topch = opench; tk->closech = closech;
		makeSyntaxRule(_ctx, tls, i+1, ne, tk->sub);
		*s = ne;
		return 1;
	}
	return 0;
}

static kbool_t makeSyntaxRule(CTX, kArray *tls, int s, int e, kArray *adst)
{
	int i;
	ksymbol_t nameid = 0;
//	dumpTokenArray(_ctx, 0, tls, s, e);
	for(i = s; i < e; i++) {
		struct _kToken *tk = tls->Wtoks[i];
		if(tk->tt == TK_INDENT) continue;
		if(tk->tt == TK_TEXT /*|| tk->tt == TK_STEXT*/) {
			if(checkNestedSyntax(_ctx, tls, &i, e, AST_PARENTHESIS, '(', ')') ||
				checkNestedSyntax(_ctx, tls, &i, e, AST_BRACKET, '[', ']') ||
				checkNestedSyntax(_ctx, tls, &i, e, AST_BRACE, '{', '}')) {
			}
			else {
				tk->tt = TK_CODE;
				tk->kw = ksymbolA(S_text(tk->text), S_size(tk->text), SYM_NEWID);
			}
			kArray_add(adst, tk);
			continue;
		}
		if(tk->tt == TK_SYMBOL || tk->tt == TK_USYMBOL) {
			if(i > 0 && tls->toks[i-1]->topch == '$') {
				tk->kw = ksymbolA(S_text(tk->text), S_size(tk->text), SYM_NEWID) | KW_PATTERN;
				tk->tt = TK_METANAME;
				if(nameid == 0) nameid = tk->kw;
				tk->nameid = nameid;
				nameid = 0;
				kArray_add(adst, tk); continue;
			}
			if(i + 1 < e && tls->toks[i+1]->topch == ':') {
				kToken *tk = tls->toks[i];
				nameid = ksymbolA(S_text(tk->text), S_size(tk->text), SYM_NEWID);
				i++;
				continue;
			}
		}
		if(tk->tt == TK_OPERATOR) {
			if(checkNestedSyntax(_ctx, tls, &i, e, AST_OPTIONAL, '[', ']')) {
				kArray_add(adst, tk);
				continue;
			}
			if(tls->toks[i]->topch == '$') continue;
		}
		Token_pERR(_ctx, tk, "illegal sugar syntax: %s", kToken_s(tk));
		return false;
	}
	return true;
}

static void parseSyntaxRule(CTX, const char *rule, kline_t uline, kArray *a)
{
	kArray *tls = ctxsugar->tokens;
	size_t pos = kArray_size(tls);
	KonohaSpace_tokenize(_ctx, NULL, rule, uline, tls);
	makeSyntaxRule(_ctx, tls, pos, kArray_size(tls), a);
	kArray_clear(tls, pos);
}

/* ------------------------------------------------------------------------ */

#ifdef __cplusplus
}
#endif
