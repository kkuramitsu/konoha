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

#ifndef ASSIGNMENT_GLUE_H_
#define ASSIGNMENT_GLUE_H_

// --------------------------------------------------------------------------

// Expr Expr.tyCheckStub(Gamma gma, int reqtyid);
static KMETHOD ExprTyCheck_assignment(CTX, ksfp_t *sfp _RIX)
{
	USING_SUGAR;
	VAR_ExprTyCheck(expr, syn, gma, reqty);
	kExpr *rexpr = kExpr_tyCheckAt(expr, 2, gma, TY_var, 0);
	kExpr *lexpr = kExpr_tyCheckAt(expr, 1, gma, rexpr->ty, TPOL_ALLOWVOID);
	if(rexpr != K_NULLEXPR && lexpr != K_NULLEXPR) {
		rexpr = kExpr_tyCheckAt(expr, 2, gma, lexpr->ty, 0);
		if(rexpr != K_NULLEXPR) {
			if(lexpr->build == TEXPR_LOCAL || lexpr->build == TEXPR_LOCAL_ || lexpr->build == TEXPR_FIELD) {
				((struct _kExpr*)expr)->build = TEXPR_LET;
				((struct _kExpr*)rexpr)->ty = lexpr->ty;
				RETURN_(expr);
			}
			if(lexpr->build == TEXPR_CALL) {  // check getter and transform to setter
				kMethod *mtd = lexpr->cons->methods[0];
				DBG_ASSERT(IS_Method(mtd));
				if((MN_isGETTER(mtd->mn) || MN_isISBOOL(mtd->mn)) && !kMethod_isStatic(mtd)) {
					ktype_t cid = lexpr->cons->exprs[1]->ty;
					mtd = kKonohaSpace_getMethodNULL(gma->genv->ks, cid, MN_toSETTER(mtd->mn));
					if(mtd != NULL) {
						KSETv(lexpr->cons->methods[0], mtd);
						kArray_add(lexpr->cons, rexpr);
						RETURN_(SUGAR Expr_tyCheckCallParams(_ctx, lexpr, mtd, gma, reqty));
					}
				}
			}
			SUGAR p(_ctx, ERR_, kExpr_uline(expr), -1, "variable name is expected");
		}
	}
	RETURN_(K_NULLEXPR);
}

// --------------------------------------------------------------------------

static	kbool_t assignment_initPackage(CTX, kKonohaSpace *ks, int argc, const char**args, kline_t pline)
{
	return true;
}

static kbool_t assignment_setupPackage(CTX, kKonohaSpace *ks, kline_t pline)
{
	return true;
}

static KMETHOD StmtTyCheck_AddAssignment(CTX, ksfp_t *sfp _RIX)
{
}

static KMETHOD StmtTyCheck_SubAssignment(CTX, ksfp_t *sfp _RIX)
{
}

static KMETHOD StmtTyCheck_MulAssignment(CTX, ksfp_t *sfp _RIX)
{
}

static KMETHOD StmtTyCheck_DivAssignment(CTX, ksfp_t *sfp _RIX)
{
}

static KMETHOD StmtTyCheck_ModAssignment(CTX, ksfp_t *sfp _RIX)
{
}

static void transfrom_oprAssignment(CTX, kArray* tls, int s, int c, int e, int kw)
{
	// a += --> a = (a) + 1
	struct _kToken *tk = (struct _kToken*)tls->toks[s];      // first token
	struct _kToken *tkOp = (struct _kToken*)tls->toks[c];    // "+="

	//change operator, a += 1 --> a = 1
	KSETv(tkOp->text, new_kString("=", 1, 0));
	tkOp->tt = TK_OPERATOR;
	tkOp->topch = '=';
	tkOp->kw = KW_LET;

	// insert new token, a = 1 --> a = (a) 1
	struct _kToken *newtk = new_W(Token, 0);
	newtk->tt = AST_PARENTHESIS; newtk->kw = AST_PARENTHESIS; newtk->uline = tk->uline;
	//newtk->topch = tk->topch; newtk->lpos = tk->closech;

	KSETv(newtk->sub, new_(TokenArray, 0));
	struct _kToken* subtk = new_W(Token, 0);
	subtk->tt = tk->tt; subtk->kw = tk->tt; subtk->uline = tk->uline; subtk->topch = tk->topch; subtk->lpos = tk->closech;
	KSETv(subtk->text, tk->text);
	kArray_add(newtk->sub, subtk);
	kArray_insert(tls, s+2, newtk);

	// insert new operator, a = (a) 1 --> a = (a) + 1
	struct _kToken *tkNewOp = new_W(Token, 0);
	KSETv(tkNewOp->text, new_kString("+", 1, 0));
	tkNewOp->tt = TK_OPERATOR;
	switch (kw) {
		case KW_ADD:
			tkNewOp->kw = KW_ADD;
			tkNewOp->topch = '+';
			break;
		case KW_SUB:
			tkNewOp->kw = KW_SUB;
			tkNewOp->topch = '-';
			break;
		case KW_MUL:
			tkNewOp->kw = KW_MUL;
			tkNewOp->topch = '*';
			break;
		case KW_DIV:
			tkNewOp->kw = KW_DIV;
			tkNewOp->topch = '/';
			break;
		case KW_MOD:
			tkNewOp->kw = KW_MOD;
			tkNewOp->topch = '%';
			break;
	}
	kArray_insert(tls, s+3, tkNewOp);
}

static KMETHOD ParseExpr_AddAssignment(CTX, ksfp_t *sfp _RIX)
{
	USING_SUGAR;
	VAR_ParseExpr(stmt, syn, tls, s, c, e);
	size_t atop = kArray_size(tls);
	transfrom_oprAssignment(_ctx, tls, s, c, e, KW_ADD);
	kExpr *expr = SUGAR Stmt_newExpr2(_ctx, stmt, tls, s, kArray_size(tls));
	kArray_clear(tls, atop);
	RETURN_(expr);
}

static KMETHOD ParseExpr_SubAssignment(CTX, ksfp_t *sfp _RIX)
{
	USING_SUGAR;
	VAR_ParseExpr(stmt, syn, tls, s, c, e);
	size_t atop = kArray_size(tls);
	transfrom_oprAssignment(_ctx, tls, s, c, e, KW_SUB);
	kExpr *expr = SUGAR Stmt_newExpr2(_ctx, stmt, tls, s, kArray_size(tls));
	kArray_clear(tls, atop);
	RETURN_(expr);
}

static KMETHOD ParseExpr_MulAssignment(CTX, ksfp_t *sfp _RIX)
{
	USING_SUGAR;
	VAR_ParseExpr(stmt, syn, tls, s, c, e);
	size_t atop = kArray_size(tls);
	transfrom_oprAssignment(_ctx, tls, s, c, e, KW_MUL);
	kExpr *expr = SUGAR Stmt_newExpr2(_ctx, stmt, tls, s, kArray_size(tls));
	kArray_clear(tls, atop);
	RETURN_(expr);
}

static KMETHOD ParseExpr_DivAssignment(CTX, ksfp_t *sfp _RIX)
{
	USING_SUGAR;
	VAR_ParseExpr(stmt, syn, tls, s, c, e);
	size_t atop = kArray_size(tls);
	transfrom_oprAssignment(_ctx, tls, s, c, e, KW_DIV);
	kExpr *expr = SUGAR Stmt_newExpr2(_ctx, stmt, tls, s, kArray_size(tls));
	kArray_clear(tls, atop);
	RETURN_(expr);
}

static KMETHOD ParseExpr_ModAssignment(CTX, ksfp_t *sfp _RIX)
{
	USING_SUGAR;
	VAR_ParseExpr(stmt, syn, tls, s, c, e);
	size_t atop = kArray_size(tls);
	transfrom_oprAssignment(_ctx, tls, s, c, e, KW_MOD);
	kExpr *expr = SUGAR Stmt_newExpr2(_ctx, stmt, tls, s, kArray_size(tls));
	kArray_clear(tls, atop);
	RETURN_(expr);
}

static kbool_t assignment_initKonohaSpace(CTX,  kKonohaSpace *ks, kline_t pline)
{
	USING_SUGAR;
	KDEFINE_SYNTAX SYNTAX[] = {
		{ TOKEN("="), /*.op2 = "*", .priority_op2 = 4096,*/ ExprTyCheck_(assignment)},
		{ TOKEN("+="), _OPLeft, /*.priority_op2 =*/ StmtTyCheck_(AddAssignment), ParseExpr_(AddAssignment), .priority_op2 = 4096,},
		{ TOKEN("-="), _OPLeft, /*.priority_op2 =*/ StmtTyCheck_(SubAssignment), ParseExpr_(SubAssignment), .priority_op2 = 4096,},
		{ TOKEN("*="), _OPLeft, /*.priority_op2 =*/ StmtTyCheck_(MulAssignment), ParseExpr_(MulAssignment), .priority_op2 = 4096,},
		{ TOKEN("/="), _OPLeft, /*.priority_op2 =*/ StmtTyCheck_(DivAssignment), ParseExpr_(DivAssignment), .priority_op2 = 4096,},
		{ TOKEN("%="), _OPLeft, /*.priority_op2 =*/ StmtTyCheck_(ModAssignment), ParseExpr_(ModAssignment), .priority_op2 = 4096,},
		{ .name = NULL, },
	};
	SUGAR KonohaSpace_defineSyntax(_ctx, ks, SYNTAX);
	return true;
}

static kbool_t assignment_setupKonohaSpace(CTX, kKonohaSpace *ks, kline_t pline)
{
	return true;
}


#endif /* ASSIGNMENT_GLUE_H_ */
