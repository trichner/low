/* Copyright (c) 2015 Thomas Richner, Fabian Schuiki */
#include <llvm-c/Core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int LLVMIntrinsicID;
extern const LLVMIntrinsicID LLVMIntrinsicIDTrap;

extern const LLVMIntrinsicID LLVMIntrinsicIDMemset;
extern const LLVMIntrinsicID LLVMIntrinsicIDMemmove;
extern const LLVMIntrinsicID LLVMIntrinsicIDMemcpy;

/*
 * Gets the function handle for llvm intrinsics.

 * The Tys parameter is for intrinsics with overloaded types (e.g., those using iAny, fAny, vAny, or iPTRAny).
 * For a declaration of an overloaded intrinsic, Tys must provide exactly
 * one type for each overloaded type in the intrinsic.
 */
LLVMValueRef LLVMGetIntrinsicByID(LLVMModuleRef mod, LLVMIntrinsicID id, LLVMTypeRef* tys, int nty);

#ifdef __cplusplus
}
#endif
