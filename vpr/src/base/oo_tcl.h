#ifndef OO_TCL_H
#define OO_TCL_H

#include <string>
#include <list>
#include <memory>

#include <tcl/tcl.h>

/**
 * \file
 * \brief Object-oriented wrappers for TCL interpreter.
 * Overview
 * ========
 * This file contains function declarations and definitionsas well as classes that allow embedding TCL interpreter in C++
 * context in a way that allows it to be extended with multiple "clients" that provide methods for managing their content.
 * 
 * A standard scenario is:
 * - // Create an instance of TclClient-derived class
 *   MyTclClient client;
 * - TclCtx ctx;
 * - // Call this before doing anything else with `ctx`.
 *   ctx.init()
 * - // This will register all methods provided by `client`
 *   ctx.add_client<MyTclClient>(client)
 * - // Read and interpret TCL code
 *   ctx.read_tcl(stream)
 * - // Process results stored by a client
 *   do_something(client.data)
 * 
 */

/**
 * \brief Base class for any exception thrown within TCLCtx.
 */
class TCL_eException {
public:
    virtual ~TCL_eException() = default;
    virtual operator std::string() const;
};

/**
 * \brief Generic exception thrown within TCLCtx. Features a message string.
 */
class TCL_eCustomException : TCL_eException {
public:
    TCL_eCustomException(std::string&& message_);
    ~TCL_eCustomException() override = default;
    operator std::string() const override;

    std::string message;
};

/**
 * \brief Exception thrown within TCLCtx if it fails during TCLCtx::init()
 */
class TCL_eFailedToInitTcl : TCL_eException {
public:
    TCL_eFailedToInitTcl(int error_code_);
    ~TCL_eFailedToInitTcl() override = default;
    operator std::string() const override;

    int error_code;
};

/**
 * \brief Exception thrown within TCLCtx when an error occurs while interpreting TCL.
 */
class TCL_eErroneousTCL : TCL_eException {
public:
    TCL_eErroneousTCL(std::string&& filename_, int line_, int column_, std::string&& message_);
    ~TCL_eErroneousTCL() override = default;
    operator std::string() const override;

    std::string filename;
    int line, column;
    std::string message;
};

void Tcl_SetStringResult(Tcl_Interp *interp, const std::string &s);

template <typename T> constexpr const Tcl_ObjType* tcl_type_of();

/**
 * \brief Get pointer to a C++ object managed within TCL-context
 * \return Pointer to a C++ object if `T` matches the type of an object. Otherwise nullptr.
 */
template <typename T>
static T* tcl_obj_getptr(Tcl_Obj* obj) {
    if (obj->typePtr != tcl_type_of<T>())
        return nullptr;
    return static_cast<T*>(obj->internalRep.twoPtrValue.ptr2);
}

/**
 * \brief Get pointer to an extra referenced C++ object.
 * \return Pointer to a C++ object.
 * 
 * This can be used to access context associated with a client, but when doing so
 * the user must ensure that whatever client is bound there, gets handled correctly.
 * Ie. when passing an object between two different clients, this should point to a
 * common interface.
 */
template <typename T>
static T* tcl_obj_get_ctx_ptr(Tcl_Obj* obj) {
    return static_cast<T*>(obj->internalRep.twoPtrValue.ptr1);
}

using TclCommandError = std::string;

/**
 * \brief Descibes a after exiting a custom TCL command implementation.
 */
enum class e_TclCommandStatus : int {
    TCL_CMD_SUCCESS,         /* Command exited with no return value. */
    TCL_CMD_SUCCESS_STRING,  /* Command exited returning a string. */
    TCL_CMD_SUCCESS_OBJECT,  /* Command exited returning a custom object. */
    TCL_CMD_FAIL             /* Command exited with a failure. Will cause a TCL_eErroneousTCL to be thrown. */
};

class TclClient {
public:
    TclClient();

    e_TclCommandStatus cmd_status;
    std::string string;
    Tcl_Obj* object;

protected:

    /* Return methods
     * Use these when returning from a TCL command implementation.
     */

    /** 
     * \brief Throw an error from a TCL command.
     */
    inline int _ret_error(TclCommandError&& error) {
        this->cmd_status = e_TclCommandStatus::TCL_CMD_FAIL;
        this->string = std::move(error);
        return TCL_ERROR;
    }

    /** 
     * \brief Return from a TCL command with no errors.
     */
    inline int _ret_ok() {
        this->cmd_status = e_TclCommandStatus::TCL_CMD_SUCCESS;
        return TCL_OK;
    }
    
    /** 
     * \brief Return a string from a TCL command with no errors.
     */
    inline int _ret_string(std::string&& string_) {
        this->cmd_status = e_TclCommandStatus::TCL_CMD_SUCCESS_STRING;
        this->string = std::move(string_);
        return TCL_OK;
    }

    /**
     * \brief Move an object to TCL-managed context and return it in TCL code.
     * \param T registered custom TCL type
     * \param this_ pointer to the concrete client. Note that it might be different than `this` within context of base class.
     * \param object_data object to be moved into TCL-managed context
     */
    template <typename T>
    int _ret_obj(void* this_, const T&& object_data) {
        const Tcl_ObjType* type = tcl_type_of<T>();
        Tcl_Obj* obj = Tcl_NewObj();
        obj->bytes = nullptr;
        obj->typePtr = type;
        obj->internalRep.twoPtrValue.ptr1 = this_;
        obj->internalRep.twoPtrValue.ptr2 = Tcl_Alloc(sizeof(T));
        new(obj->internalRep.twoPtrValue.ptr2) T(std::move(object_data));
        if (type->updateStringProc != nullptr)
            type->updateStringProc(obj);
        
        this->object = obj;
        this->cmd_status = e_TclCommandStatus::TCL_CMD_SUCCESS_OBJECT;

        return TCL_OK;
    }
};

/* Default implementations for handling TCL-managed C++ objects */
template <typename T>
static void tcl_obj_free(Tcl_Obj* obj) {
    T* obj_data = tcl_obj_getptr<T>(obj);
    obj_data->~T();
    Tcl_Free(reinterpret_cast<char*>(obj_data));
}
void tcl_obj_dup(Tcl_Obj* src, Tcl_Obj* dst);
void tcl_set_obj_string(Tcl_Obj* obj, const std::string& str);
int tcl_set_from_none(Tcl_Interp *tcl_interp, Tcl_Obj *obj);

void tcl_init();

/**
 * \brief Provide TCL runtime for parsing XDC files.
 * Allows extending TCL language with new types which are associated with C++ types and new methods provided by objects of TclClient-derived classes.
 * 
 * `TclCtx` binds a TCL context to the lifetime of a C++ object, so once TclCtx::~TclCtx gets called, TCL context will be cleaned-up.
 * It is illegal to call any method of `TclCtx` othar than its destructor after destroying a bound client.
 */
class TclCtx {
public:
    TclCtx();

    ~TclCtx();

    /**
     * \brief Initialize a newly created instance of TclCtx. It's illegal to call any other method before calling this one.
     */
    void init();

    /**
     * \brief Read and interpret a TCL script within this context.
     */
    void read_tcl(std::istream& tcl_stream);

    /**
     * \brief Add a client class that provides methods to interface with a context managed by it
     */
    template <typename C>
        void add_tcl_client(C& client) {
        auto* client_container = new TclClientContainer<C>(client);
        auto register_command = [&](const char* name, int(C::*method)(int objc, Tcl_Obj* const objvp[])) {
            client_container->methods.push_front(TclMethodDispatch<C>(method, client));
            auto& md = client_container->methods.front();
            Tcl_CreateObjCommand(this->_tcl_interp, name, TclCtx::_tcl_do_method, static_cast<TclMethodDispatchBase*>(&md), nullptr);
        };

        client.register_methods(register_command);

        this->_tcl_clients.push_front(std::unique_ptr<TclClientContainerBase>(static_cast<TclClientContainerBase*>(client_container)));
    }

    /**
     * \brief Add a custom type to TCL.
     * 
     * The type has to be moveable in order to allow passing it between Tcl-managed cotext and C++ code.
     * Use REGISTER_TCL_TYPE macro to associate type T with a static struct instance describing the Tcl type.
     */
    template <typename T>
    inline void add_tcl_type() {
        Tcl_RegisterObjType(tcl_type_of<T>());
    }

protected:

    /**
     * \brief Tcl by default will interpret bus indices as calls to commands
     *        (eg. in[0] gets interpreted as call to command `i` with argument being a
     *        result of calling `0`). This overrides this behaviour.
     * 
     * TODO: This is SDC/XDC-specific and should be handled somewhere else.
     */
    int _fix_port_indexing();

    /**
     * \brief Provide an interface to call a client method.
     */
    struct TclMethodDispatchBase {
        inline TclMethodDispatchBase(TclClient& client_) : client(client_) {}
        virtual ~TclMethodDispatchBase() {}
        virtual int do_method(int objc, Tcl_Obj* const objvp[]) = 0;
        TclClient& client;
    };

    /**
     * \brief Wrap a call-by-PTM (pointer-to-method) of a concrete TclClient-derived class as a virtual function of TclMethodDispatchBase.
     * This allows us to avoid making methods defined within TclClient-derived virtual methods of TclClient.
     */
    template <typename C>
    struct TclMethodDispatch : TclMethodDispatchBase {
        typedef int(C::*TclMethod)(int objc, Tcl_Obj* const objvp[]);
        inline TclMethodDispatch(TclMethod method_, TclClient& client_) : TclMethodDispatchBase(client_), method(method_) {}
        virtual ~TclMethodDispatch() = default;

        virtual int do_method(int objc, Tcl_Obj* const objvp[]) {
            auto& casted_client = static_cast<C&>(this->client);
            return (casted_client.*(this->method))(objc, objvp);
        }

        TclMethod method;
    };

    struct TclClientContainerBase {};

    /**
     * \brief Store a reference to a client along with method dispatch list associated with that client.
     */
    template <typename C>
    struct TclClientContainer : TclClientContainerBase {
        inline TclClientContainer(C& client_) : client(client_) {}
        C& client;
        /* Use list to guarantee the validity of references */
        std::list<TclMethodDispatch<C>> methods;
    };

    static int _tcl_do_method(
        ClientData cd,
        Tcl_Interp* tcl_interp,
        int objc,
        Tcl_Obj* const objvp[]
    );

    Tcl_Interp* _tcl_interp;
    /* Use list to guarantee the validity of references */
    std::list<std::unique_ptr<TclClientContainerBase>> _tcl_clients;

private:
#ifdef DEBUG
    inline void _debug_init() const {
        VTR_ASSERT(this->_init);
    }

    bool _init;
#else
    inline void _debug_init() const {}
#endif
};

#define DECLARE_TCL_TYPE(cxx_type, tcl_type)                  \
    extern const Tcl_ObjType tcl_type;                        \
    template <>                                               \
    constexpr const Tcl_ObjType* tcl_type_of<cxx_type>() {    \
        return &tcl_type;                                     \
    }                                                         \

/**
 * \brief Associate a C++ struct/class with a `Tcl_ObjType`.
 * \param cxx_type C++ type name.
 * \param tcl_type pointer to statically-allocated Tcl_ObjType.
 */
#define REGISTER_TCL_TYPE_W_STR_UPDATE(cxx_type, tcl_type)    \
    const Tcl_ObjType tcl_type = (Tcl_ObjType){               \
        .name = #cxx_type,                                    \
        .freeIntRepProc = tcl_obj_free<cxx_type>,             \
        .dupIntRepProc = tcl_obj_dup,                         \
        .updateStringProc = [](Tcl_Obj* obj)

#define END_REGISTER_TCL_TYPE ,                               \
    .setFromAnyProc = tcl_set_from_none                       \
}

#endif