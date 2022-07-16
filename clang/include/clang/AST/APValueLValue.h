
#ifndef LLVM_CLANG_AST_APVALUE_LVALUE_H
#define LLVM_CLANG_AST_APVALUE_LVALUE_H

#include "clang/Basic/LLVM.h"
#include "APValue.h"
#include "llvm/ADT/PointerIntPair.h"

namespace clang
{

/// Symbolic representation of a dynamic allocation.
class DynamicAllocLValue {

  // if the last bit is 1, this isn't a pointer
  std::size_t Index; 
  
public: 
  
  DynamicAllocLValue(APValue* V) {
    Index = reinterpret_cast<std::size_t>(V);
  }
  
  DynamicAllocLValue() : Index(1U) {}
  explicit DynamicAllocLValue(unsigned Index) : Index(((Index + 1) << 1) | 1U) {}
  unsigned getIndex() const { return (Index >> 1) - 1; }
  
  explicit operator bool() const { return (Index >> 1) != 0; }
  
  APValue* getPointer() const 
  { 
    assert( isPointer() );
    // we static_assert that an APValue has an alignment of 8 in APValue.cpp
    return reinterpret_cast<APValue*>(Index);
  }
  
  bool isPointer() const { return not (Index & 1U); }
  
  void *getOpaqueValue() {
    return reinterpret_cast<void *>(static_cast<uintptr_t>(Index)
                                    << NumLowBitsAvailable);
  }
  
  static DynamicAllocLValue getFromOpaqueValue(void *Value) {
    DynamicAllocLValue V;
    V.Index = reinterpret_cast<uintptr_t>(Value) >> NumLowBitsAvailable;
    return V;
  }

  static unsigned getMaxIndex() {
    return (std::numeric_limits<unsigned>::max() >> NumLowBitsAvailable) - 1;
  }

  static constexpr int NumLowBitsAvailable = 2;
};

} // clang

namespace llvm
{

template<> struct PointerLikeTypeTraits<clang::DynamicAllocLValue> {
  static void *getAsVoidPointer(clang::DynamicAllocLValue V) {
    return V.getOpaqueValue();
  }
  static clang::DynamicAllocLValue getFromVoidPointer(void *P) {
    return clang::DynamicAllocLValue::getFromOpaqueValue(P);
  }
  static constexpr int NumLowBitsAvailable =
      clang::DynamicAllocLValue::NumLowBitsAvailable;
};

} // end namespace llvm

namespace clang
{

class APValue::LValueBase 
{
    typedef llvm::PointerUnion<const ValueDecl *, const Expr *, TypeInfoLValue,
                               DynamicAllocLValue>
        PtrTy;

  public:
    LValueBase() : Local{} {}
    LValueBase(const ValueDecl *P, unsigned I = 0, unsigned V = 0);
    LValueBase(const Expr *P, unsigned I = 0, unsigned V = 0);
    static LValueBase getDynamicAlloc(DynamicAllocLValue LV, QualType Type);
    static LValueBase getTypeInfo(TypeInfoLValue LV, QualType TypeInfo);

    void Profile(llvm::FoldingSetNodeID &ID) const;
    
    void replaceWith(APValue* V);
    
    template <class T>
    bool is() const { return Ptr.is<T>(); }

    template <class T>
    T get() const { return Ptr.get<T>(); }

    template <class T>
    T dyn_cast() const { return Ptr.dyn_cast<T>(); }

    void *getOpaqueValue() const;

    bool isNull() const;

    explicit operator bool() const;

    unsigned getCallIndex() const;
    unsigned getVersion() const;
    QualType getTypeInfoType() const;
    QualType getDynamicAllocType() const;

    QualType getType() const;

    friend bool operator==(const LValueBase &LHS, const LValueBase &RHS);
    friend bool operator!=(const LValueBase &LHS, const LValueBase &RHS) {
      return !(LHS == RHS);
    }
    friend llvm::hash_code hash_value(const LValueBase &Base);
    friend struct llvm::DenseMapInfo<LValueBase>;

  private:
    PtrTy Ptr;
    struct LocalState {
      unsigned CallIndex, Version;
    };
    union {
      LocalState Local;
      /// The type std::type_info, if this is a TypeInfoLValue.
      void *TypeInfoType;
      /// The QualType, if this is a DynamicAllocLValue.
      void *DynamicAllocType;
    };
};
  
} // end namespace clang

namespace llvm {
template<> struct DenseMapInfo<clang::APValue::LValueBase> {
  static clang::APValue::LValueBase getEmptyKey();
  static clang::APValue::LValueBase getTombstoneKey();
  static unsigned getHashValue(const clang::APValue::LValueBase &Base);
  static bool isEqual(const clang::APValue::LValueBase &LHS,
                      const clang::APValue::LValueBase &RHS);
};
}

#endif
