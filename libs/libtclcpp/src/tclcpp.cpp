#include "tclcpp.h"

#include <functional>
#include <vector>
#include <list>
#include <tcl/tcl.h>
#include <cstring>
#include <sstream>

/* TCL ERRORS */
#define OV_TOSTRING(c, e) \
    c::operator std::string() const { return (e); }

OV_TOSTRING(TCL_eException, "Unknown error")

TCL_eCustomException::TCL_eCustomException(std::string&& message_)
    : message(std::move(message_)) {}
TCL_eCustomException::operator std::string() const { return (this->message); }

TCL_eFailedToInitTcl::TCL_eFailedToInitTcl(int error_code_)
    : error_code(error_code_) {}
OV_TOSTRING(TCL_eFailedToInitTcl, "Can't initialize TCL (code " + std::to_string(error_code) + ")")

TCL_eErroneousTCL::TCL_eErroneousTCL(
    std::string&& filename_,
    int line_,
    int column_,
    std::string&& message_)
    : filename(std::move(filename_))
    , line(line_)
    , column(column_)
    , message(message_) {}
OV_TOSTRING(TCL_eErroneousTCL, this->filename + ":" + std::to_string(this->line) + "," + std::to_string(this->column) + ": " + this->message)

void Tcl_SetStringResult(Tcl_Interp* interp, const std::string& s) {
    char* copy = Tcl_Alloc(s.length() + 1);
    std::strcpy(copy, s.c_str());
    Tcl_SetResult(interp, copy, TCL_DYNAMIC);
}

TclClient::TclClient()
    : cmd_status(e_TclCommandStatus::TCL_CMD_SUCCESS)
    , string("No errors") {}

void tcl_obj_dup(Tcl_Obj* src, Tcl_Obj* dst) {
    dst->internalRep.twoPtrValue = src->internalRep.twoPtrValue;
    dst->typePtr = src->typePtr;
    dst->bytes = nullptr;
    dst->length = 0;
}

void tcl_set_obj_string(Tcl_Obj* obj, const std::string& str) {
    if (obj->bytes != nullptr)
        Tcl_Free(obj->bytes);
    obj->bytes = Tcl_Alloc(str.length() + 1);
    obj->length = str.length();
    std::strcpy(obj->bytes, str.c_str());
}

int tcl_set_from_none(Tcl_Interp* tcl_interp, Tcl_Obj* obj) {
    (void)(obj); /* Surpress "unused parameter" macro */
    /* TODO: Better error message */
    Tcl_SetStringResult(tcl_interp, "Attempted an invalid conversion.");
    return TCL_ERROR;
}

TclCtx::TclCtx() {
    this->_tcl_interp = Tcl_CreateInterp();
#ifdef DEBUG
    this->_init = false;
#endif
}

TclCtx::~TclCtx() {
    Tcl_DeleteInterp(this->_tcl_interp);
}

void TclCtx::_init() {
    int error;

    if ((error = Tcl_Init(this->_tcl_interp)) != TCL_OK)
        throw TCL_eFailedToInitTcl(error);

#ifdef DEBUG
    this->_init = true;
#endif
}

void TclCtx::read_tcl(std::istream& tcl_stream) {
    int error;

    this->_debug_init();

    std::ostringstream os;
    tcl_stream >> os.rdbuf();
    std::string tcl = os.str();

    error = Tcl_Eval(this->_tcl_interp, tcl.c_str());
    /* TODO: More precise error */
    if (error != TCL_OK) {
        int error_line = Tcl_GetErrorLine(this->_tcl_interp);
        const char* msg = Tcl_GetStringResult(this->_tcl_interp);
        throw TCL_eErroneousTCL("<unknown file>", error_line, 0, std::string(msg));
    }
}

int TclCtx::_tcl_do_method(
    ClientData cd,
    Tcl_Interp* tcl_interp,
    int objc,
    Tcl_Obj* const objvp[]) {
    TclMethodDispatchBase* d = static_cast<TclMethodDispatchBase*>(cd);
    d->do_method(objc, objvp);

    switch (d->client.cmd_status) {
        case e_TclCommandStatus::TCL_CMD_FAIL:
            Tcl_SetStringResult(tcl_interp, d->client.string);
            return TCL_ERROR;
        case e_TclCommandStatus::TCL_CMD_SUCCESS_STRING:
            Tcl_SetStringResult(tcl_interp, d->client.string);
            return TCL_OK;
        case e_TclCommandStatus::TCL_CMD_SUCCESS_OBJECT:
        case e_TclCommandStatus::TCL_CMD_SUCCESS_LIST:
            Tcl_SetObjResult(tcl_interp, d->client.object);
        default:
            break;
    }
    return TCL_OK;
}
