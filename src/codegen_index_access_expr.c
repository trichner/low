/* Copyright (c) 2015 Fabian Schuiki */
#include "codegen_internal.h"
#include "llvm_intrinsics.h"


static void
build_assert(codegen_t *self,codegen_context_t *context, LLVMValueRef cond){
	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(self->builder));
	LLVMBasicBlockRef true_block = LLVMAppendBasicBlock(func, "iftrue");
	LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(func, "ifexit");

	cond = LLVMBuildNot(self->builder,cond,"");
	LLVMBuildCondBr(self->builder, cond, true_block, exit_block);

	LLVMPositionBuilderAtEnd(self->builder, true_block);

	LLVMValueRef fn = LLVMGetIntrinsicByID(self->module,LLVMIntrinsicIDTrap,0,0);

	assert(fn && "Intrinsic function not found!");

	LLVMBuildCall(self->builder,fn,0,0,"");

	LLVMBuildBr(self->builder, exit_block);

	LLVMPositionBuilderAtEnd(self->builder, exit_block);
}


PREPARE_TYPE(index_access_expr) {
	type_t int_type = { .kind = AST_INTEGER_TYPE, .pointer = 0, .width = 64 };
	prepare_expr(self, context, expr->index_access.target, 0);
	prepare_expr(self, context, expr->index_access.index, &int_type);
	type_t *target = &expr->index_access.target->type;
	if (target->pointer > 0) {
		type_copy(&expr->type, target);
		--expr->type.pointer;
	} else if (target->kind == AST_ARRAY_TYPE) {
		type_copy(&expr->type, target->array.type);
	} else if (target->kind == AST_SLICE_TYPE) {
		type_copy(&expr->type, target->slice.type);
	} else {
		derror(&expr->loc, "cannot index into non-pointer\n");
	}
}


CODEGEN_EXPR(index_access_expr) {
	LLVMValueRef target = codegen_expr(self, context, expr->index_access.target, 0, 0);
	LLVMValueRef index = codegen_expr(self, context, expr->index_access.index, 0, 0);
	if (expr->index_access.index->type.kind != AST_INTEGER_TYPE){
		derror(&expr->index_access.index->loc, "index needs to be an integer\n");
	}

	LLVMValueRef ptr;
	if (expr->index_access.target->type.pointer > 0) {
		ptr = LLVMBuildInBoundsGEP(self->builder, target, &index, 1, "");
	} else if (expr->index_access.target->type.kind == AST_ARRAY_TYPE) {
		ptr = LLVMBuildInBoundsGEP(self->builder, target, (LLVMValueRef[]){LLVMConstNull(LLVMInt32Type()), index}, 2, "");
	} else if (expr->index_access.target->type.kind == AST_SLICE_TYPE) {

		LLVMValueRef capptr = LLVMBuildStructGEP(self->builder, target, 2, "");
		LLVMValueRef cap = LLVMBuildLoad(self->builder,capptr,"");

		LLVMTypeRef dst = LLVMTypeOf(cap);

		index = LLVMBuildIntCast(self->builder, index, dst,"");

		// check idx vs cap
		LLVMValueRef oob = LLVMBuildICmp(self->builder, LLVMIntULT, index, cap, "");
		build_assert(self,context,oob);

		LLVMValueRef arrptrptr = LLVMBuildStructGEP(self->builder, target, 0, "");
		LLVMValueRef arrptr = LLVMBuildLoad(self->builder,arrptrptr,"");

		ptr = LLVMBuildInBoundsGEP(self->builder, arrptr, (LLVMValueRef[]){LLVMConstNull(LLVMInt32Type()), index}, 2, "");

	} else {
		derror(&expr->loc, "cannot index into non-pointer or non-array");
	}
	return lvalue ? ptr : LLVMBuildLoad(self->builder, ptr, "");
}

PREPARE_TYPE(index_slice_expr) {
	type_t int_type = { .kind = AST_INTEGER_TYPE, .pointer = 0, .width = 64 };

	prepare_expr(self, context, expr->index_slice.target, 0);
	if(expr->index_slice.start){
		prepare_expr(self, context, expr->index_slice.start, &int_type);
	}
	if(expr->index_slice.end){
		prepare_expr(self, context, expr->index_slice.end, &int_type);
	}

	type_t *target = &expr->index_slice.target->type;
	if(target->kind != AST_SLICE_TYPE){
		derror(&expr->loc, "can only slice slices");
	}
	type_copy(&expr->type, target);
}

CODEGEN_EXPR(index_slice_expr) {
	LLVMValueRef target = codegen_expr(self, context, expr->index_slice.target, 0, 0);

	LLVMValueRef zero = LLVMConstNull(LLVMInt64Type());

	LLVMValueRef start 	= 0;
	LLVMValueRef end 	= 0;
	if(expr->index_slice.start){
		if (expr->index_slice.start->type.kind != AST_INTEGER_TYPE){
			derror(&expr->index_slice.start->loc, "index needs to be an integer\n");
		}
		start = codegen_expr(self, context, expr->index_slice.start, 0, 0);

	}else{
		start = zero;
	}
	if(expr->index_slice.end){
		if (expr->index_slice.end->type.kind != AST_INTEGER_TYPE){
			derror(&expr->index_slice.end->loc, "index needs to be an integer\n");
		}
		end = codegen_expr(self, context, expr->index_slice.end, 0, 0);
	}else{
		end = zero;
	}

	//------------
	LLVMValueRef capptr = LLVMBuildStructGEP(self->builder, target, 2, "");
	LLVMValueRef cap = LLVMBuildLoad(self->builder,capptr,"");

	LLVMValueRef lenptr = LLVMBuildStructGEP(self->builder, target, 1, "");
	LLVMValueRef len = LLVMBuildLoad(self->builder,lenptr,"");

	LLVMTypeRef dst = LLVMTypeOf(cap);

	start = LLVMBuildIntCast(self->builder, start, dst,"");
	end = LLVMBuildIntCast(self->builder, end, dst,"");

	// ncap = end - start
	// nlen = max(len - start,0)
	// narrptr = arrptr + start

	//---- calc new len/cap
	LLVMValueRef ncap = LLVMBuildSub(self->builder, end, start, "");
	LLVMValueRef nlen = LLVMBuildSub(self->builder, len, start, "");

	// conditional max(0,nlen)
	LLVMValueRef cond = LLVMBuildICmp(self->builder, LLVMIntSLT,nlen,zero,"min");

	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(self->builder));
	LLVMBasicBlockRef true_block = LLVMAppendBasicBlock(func, "iftrue");
	LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(func, "ifexit");

	LLVMBuildCondBr(self->builder, cond, true_block, exit_block);
	LLVMPositionBuilderAtEnd(self->builder, true_block);

	nlen = zero;

	LLVMBuildBr(self->builder, exit_block);
	LLVMPositionBuilderAtEnd(self->builder, exit_block);

	//---- calc new array ptr
	LLVMValueRef arrptrptr = LLVMBuildStructGEP(self->builder, target, 0, "");
	LLVMValueRef arrptr = LLVMBuildLoad(self->builder,arrptrptr,"");
	LLVMValueRef ptr = LLVMBuildInBoundsGEP(self->builder, arrptr, (LLVMValueRef[]){LLVMConstNull(LLVMInt32Type()), start}, 2, "");

	// cast to correct pointer type
	LLVMTypeRef array_type = LLVMTypeOf(arrptr);
	ptr = LLVMBuildPointerCast(self->builder,ptr,array_type,"");

	LLVMValueRef baseptrptr = LLVMBuildStructGEP(self->builder, target, 3, "");
	LLVMValueRef baseptr = LLVMBuildLoad(self->builder,baseptrptr,"");

	//---- assemble struct
	LLVMValueRef oldslice = LLVMBuildLoad(self->builder,target,"old_slice");
	LLVMValueRef slice = LLVMConstNull(LLVMTypeOf(oldslice));

	slice = LLVMBuildInsertValue(self->builder, slice, ptr, 0, "s_arrptr");
	slice = LLVMBuildInsertValue(self->builder, slice, nlen, 1, "s_len");
	slice = LLVMBuildInsertValue(self->builder, slice, ncap, 2, "s_cap");
	slice = LLVMBuildInsertValue(self->builder, slice, baseptr, 3, "s_baseptr");

	return slice;
}