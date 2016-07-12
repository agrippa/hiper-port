#ifndef OMP_VAR_INFO_H
#define OMP_VAR_INFO_H

#include "clang/AST/Decl.h"

enum CAPTURE_TYPE {
    NONE = 0,
    SHARED,
    PRIVATE,
    FIRSTPRIVATE,
    LASTPRIVATE
};

class OMPVarInfo {
    private:
        clang::ValueDecl *decl;
        enum CAPTURE_TYPE type;
        bool isGlobal;

    public:
        OMPVarInfo();
        OMPVarInfo(const OMPVarInfo& other);
        OMPVarInfo(clang::ValueDecl *decl, enum CAPTURE_TYPE type,
                bool isGlobal);

        enum CAPTURE_TYPE getType();
        clang::ValueDecl *getDecl();
        bool checkIsGlobal();

        bool isPassedByReference();
};

#endif
