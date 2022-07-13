
#ifndef LLVM_CLANG_AST_APVALUE_WITH_HEAP_H
#define LLVM_CLANG_AST_APVALUE_WITH_HEAP_H

#include "clang/AST/APValue.h"
#include <map> 
#include <memory>

namespace clang
{

class Expr;

struct DynAlloc {
  /// The value of this heap-allocated object.
  APValue Value;
  /// The allocating expression; used for diagnostics. Either a CXXNewExpr
  /// or a CallExpr (the latter is for direct calls to operator new inside
  /// std::allocator<T>::allocate).
  const Expr *AllocExpr = nullptr;

  enum Kind {
    New,
    ArrayNew,
    StdAllocator
  };

  /// Get the kind of the allocation. This must match between allocation
  /// and deallocation.
  // defined in ExprConstant.cpp
  Kind getKind() const;
};

struct DynAllocOrder {
  bool operator()(DynamicAllocLValue L, DynamicAllocLValue R) const {
    return L.getIndex() < R.getIndex();
  }
};

using HeapAllocMap = std::map<DynamicAllocLValue, DynAlloc, DynAllocOrder>;
 
/// A value produced by a constant expression which contains 
/// subobjects pointing to virtually dynamically allocated memory.
/// This bundle an APValue with the pseudo heaps chunks

class APValueWithHeap
{
  struct Data 
  {
     APValue value;
     HeapAllocMap map;
  };
  
  std::unique_ptr<Data> data;
   
  public : 
  
  APValueWithHeap(APValue&& value, HeapAllocMap&& map)
  : data{ std::make_unique<Data>(Data{decltype(value)(value), decltype(map)(map)}) }
  {}
  
  APValueWithHeap(const APValueWithHeap& O) noexcept
  : data { std::make_unique<Data>(*O.data) }
  {}
  
  APValueWithHeap(APValueWithHeap&& O) noexcept = default;
  
  auto& value()       { return data->value; }
  auto& value() const { return data->value; }
  auto& heap()        { return data->map;   }
  auto& heap() const  { return data->map;   }
};

} // end clang namespace

#endif