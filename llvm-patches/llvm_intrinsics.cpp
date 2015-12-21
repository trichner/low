
#include "llvm_intrinsics.h"
#include <llvm/IR/DataLayout.h>
#include "llvm/IR/Module.h"
#include <llvm/IR/Function.h>
#include <llvm/IR/Intrinsics.h>
//#include <std>

const LLVMIntrinsicID LLVMIntrinsicIDTrap = llvm::Intrinsic::trap;

const LLVMIntrinsicID LLVMIntrinsicIDMemset = llvm::Intrinsic::memset;
const LLVMIntrinsicID LLVMIntrinsicIDMemmove = llvm::Intrinsic::memmove;
const LLVMIntrinsicID LLVMIntrinsicIDMemcpy = llvm::Intrinsic::memcpy;

LLVMValueRef LLVMGetIntrinsicByID(LLVMModuleRef mod, LLVMIntrinsicID id, LLVMTypeRef *tys,int nty){
	std::vector<llvm::Type *> arg_types;
	for(;nty>0;nty--){
		arg_types.push_back(llvm::unwrap(*tys));
		tys++;
	}
	return llvm::wrap(llvm::Intrinsic::getDeclaration(llvm::unwrap(mod), (llvm::Intrinsic::ID) id, arg_types));
}