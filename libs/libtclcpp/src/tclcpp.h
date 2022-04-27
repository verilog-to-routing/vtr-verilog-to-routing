#ifndef TCLCPP_H
#define TCLCPP_H

#include <string>
#include <list>
#include <vector>
#include <memory>
#include <functional>
#include <cstring>
#include <type_traits>

#include <tcl/tcl.h>

/**
 * @file
 * @brief Object-oriented wrappers for TCL interpreter.
 * Overview
 * ========
 * This file contains function declarations and definitionsas well as classes that allow embedding TCL interpreter in C++
 * context in a way that allows it to be extended with multiple "clients" that provide methods for managing their content.
 * 
 * A standard scenario is:
 * - // Create an instance of TclClient-derived class
 *   MyTclClient client;
 * - // Create a TclCtx context and run code within its lifetime.
 *   TclCtx::with_ctx([&](TclCtx& ctx) {;
 * -   // Call this before doing anything else with `ctx`.
 *     ctx.init()
 * -   // This will register all methods provided by `client`
 *     ctx.add_client<MyTclClient>(client)
 * -   // Read and interpret TCL code
 *     ctx.read_tcl(stream)
 * - // TclCtx gets destroyd at the end of this call
 *   });
 * - // Process results stored by a client
 *   do_something(client.data)
 */

/**
 * @brief Base class for any exception thrown within TCLCtx.
 */
class TCL_eException {
  public:
    virtual ~TCL_eException() = default;
    virtual operator std::string() const;
};

/**
 * @brief Generic exception thrown within TCLCtx. Features a message string.
 */
class TCL_eCustomException : TCL_eException {
  public:
    TCL_eCustomException(std::string&& message_);
    ~TCL_eCustomException() override = default;
    operator std::string() const override;

    std::string message;
};

/**
 * @brief Exception thrown within TCLCtx if it fails during TCLCtx::init()
 */
class TCL_eFailedToInitTcl : TCL_eException {
  public:
    TCL_eFailedToInitTcl(int error_code_);
    ~TCL_eFailedToInitTcl() override = default;
    operator std::string() const override;

    int error_code;
};

/**
 * @brief Exception thrown within TCLCtx when an error occurs while interpreting TCL.
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

void Tcl_SetStringResult(Tcl_Interp* interp, const std::string& s);

template<typename T>
constexpr const Tcl_ObjType* tcl_type_of();

/**
 * @brief Get pointer to a C++ object managed within TCL-context
 * @return Pointer to a C++ object if `T` matches the type of an object. Otherwise nullptr.
 */
template<typename T>
static T* tcl_obj_getptr(Tcl_Obj* obj) {
    if (obj->typePtr != tcl_type_of<T>())
        return nullptr;
    return static_cast<T*>(obj->internalRep.twoPtrValue.ptr2);
}

/**
 * @brief Get pointer to an extra referenced C++ object.
 * @return Pointer to a C++ object.
 * 
 * This can be used to access context associated with a client, but when doing so
 * the user must ensure that whatever client is bound there, gets handled correctly.
 * Ie. when passing an object between two different clients, this should point to a
 * common interface.
 */
template<typename T>
static T* tcl_obj_get_ctx_ptr(Tcl_Obj* obj) {
    return static_cast<T*>(obj->internalRep.twoPtrValue.ptr1);
}

using TclCommandError = std::string;

/**
 * @brief Descibes a state after exiting a custom TCL command implementation.
 */
enum class e_TclCommandStatus : int {
    TCL_CMD_SUCCESS,        /* Command exited with no return value. */
    TCL_CMD_SUCCESS_STRING, /* Command exited returning a string. */
    TCL_CMD_SUCCESS_OBJECT, /* Command exited returning a custom object. */
    TCL_CMD_SUCCESS_LIST,   /* Command exited returning a list. */
    TCL_CMD_FAIL            /* Command exited with a failure. Will cause a TCL_eErroneousTCL to be thrown. */
};

template<typename T, class = typename std::enable_if<std::is_move_constructible<T>::value>::type>
Tcl_Obj* Tcl_MoveCppObject(void* ctx, T& object_data) {
    const Tcl_ObjType* type = tcl_type_of<T>();
    Tcl_Obj* obj = Tcl_NewObj();
    obj->bytes = nullptr;
    obj->typePtr = type;
    obj->internalRep.twoPtrValue.ptr1 = ctx;
    obj->internalRep.twoPtrValue.ptr2 = Tcl_Alloc(sizeof(T));
    new (obj->internalRep.twoPtrValue.ptr2) T(std::move(object_data));
    if (type->updateStringProc != nullptr)
        type->updateStringProc(obj);

    return obj;
}

/**
 * @brief C++ wrapper for TCL lists of TCL-managed C++ objects. Allows easy iteration and construction from C++ containers.
 */
template<typename T>
class TclList {
  public:
    /**
     * @brief Create empty TCL list
     */
    TclList(Tcl_Interp* interp)
        : _interp(interp)
        , _obj(Tcl_NewListObj(0, nullptr)) {}

    /**
     * @brief Create a TCL list from a C++ container
     * Client must point to the concrete TclClient instance.
     */
    template<typename Container>
    TclList(Tcl_Interp* interp, void* client, Container&& elements) {
        std::vector<Tcl_Obj*> tcl_objs;
        for (auto&& elem : elements) {
            Tcl_Obj* obj = Tcl_MoveCppObject(client, elem);
            tcl_objs.push_back(obj);
        }

        this->_obj = Tcl_NewListObj(tcl_objs.size(), &tcl_objs[0]);
    }

    /**
     * @brief Import a TCL list from Tcl_Obj of list type, or create one-element list from
     *        object of any other type.
     */
    TclList(Tcl_Interp* interp, Tcl_Obj* obj)
        : _interp(interp) {
        const Tcl_ObjType* obj_type = obj->typePtr;
        if (obj_type == nullptr)
            return;

        if (obj_type == nullptr || std::strcmp(obj_type->name, "list")) {
            Tcl_Obj* item;
            this->_obj = Tcl_NewListObj(1, &obj);
        } else {
            this->_obj = obj;
        }
    }

    /**
     * @brief Get the raw TCL_Obj representing the list.
     */
    Tcl_Obj* tcl_obj() {
        return this->_obj;
    }

    /**
     * @brief Iterator
     * Dereferencing it yields a pointer to a C++ object
     * 
     * TODO: If we ever need to handle a TCL list of TCL lists this won't work. It might be possible to use
     * template spcializations to derive a class with alternative implementation of `operator*()` that would
     * return `TclList<T>` for `TclList<TclList<T>>::Iterator::operaotr*()`.
     * Primitive TCL types might also need a specialized handling.
     */
    template<class dummy>
    class Iterator {
      public:
        Iterator& operator=(const Iterator& other) {
            this->_idx = other->_idx;
            return *this;
        }

        Iterator& operator++() {
            this->_idx++;
            return *this;
        }

        bool operator!=(const Iterator& other) {
            return (this->_interp != other._interp) || (this->_obj != other._obj) || (this->_idx != other._idx);
        }

        T* operator*() const {
            Tcl_Obj* objp;
            Tcl_ListObjIndex(this->_interp, this->_obj, this->_idx, &objp);
            return tcl_obj_getptr<T>(objp);
        }

        Iterator(Tcl_Interp* interp, Tcl_Obj* obj, int idx)
            : _interp(interp)
            , _obj(obj)
            , _idx(idx) {}

      protected:
        Tcl_Interp* _interp;
        int _idx;      /* Currently visited index on the list */
        Tcl_Obj* _obj; /* Underlying Tcl_Obj representing the list */
    };

    Iterator<void> begin() {
        return Iterator<void>(this->_interp, this->_obj, 0);
    }

    Iterator<void> end() {
        int count;
        Tcl_ListObjLength(this->_interp, this->_obj, &count);
        return Iterator<void>(this->_interp, this->_obj, count);
    }

    /**
     * @brief Get number of elements on the list.
     */
    size_t size() const {
        int len;
        Tcl_ListObjLength(this->_interp, this->_obj, &len);
        return size_t(len);
    }

  protected:
    Tcl_Interp* _interp;
    Tcl_Obj* _obj; /* Underlying Tcl_Obj representing the list */
};

class TclCtx;

/**
 * @brief Provide a C++ state with TCL interface.
 * 
 * This class should be a base for a Client class to implement a TCL interface.
 * 
 * Commands need to be registered within
 *   template <typename F>
 *   void register_methods(F register_method)
 * method template.
 * 
 * Commands are implemented as methods of TclClient-derived class:
 *   int(DerivedClient::*)(int objc, Tcl_Obj* const objvp[]))
 * 
 * Use TclClient::_ret_* methods to return from a command implementation.
 */
class TclClient {
  public:
    e_TclCommandStatus cmd_status;
    std::string string;
    Tcl_Obj* object;

    /**
     * Define this method template in a derived TclClient:
     * 
     * template <typename F>
     * void register_methods(F register_method)
     * 
     * F should be a functor of acting as the following type:
     * void(*)(const char* name, int(DerivedClient::*)(int objc, Tcl_Obj* const objvp[]))
     * 
     * (alternatively you can define a method taking std::function)
     * 
     * Eg.:
     * template <typename F>
     * void DerivedClient::register_methods(F register_method) {
     *   register_method("my_custom_command", DerivedClient::my_custom_command);
     *   register_method("my_other_custom_command", DerivedClient::my_other_custom_command);
     * }
     * 
     * In the future if C++20 is going to be adopted as a standard for VPR, this should be
     * handled using a concept.
     */

  protected:
    /* TclClient must be derived */
    TclClient();

    /* TclClient must be immovable to prevent invalidating any references to it stored within TCL objects */
    TclClient(TclClient&& other) = delete;
    /* TclClient shouldn't be copied, because that would require duplicating this->object and binding it to
     * the new copy of a client. This might be an unwanted behaviour. */
    TclClient(const TclClient& other) = delete;
    TclClient& operator=(const TclClient& other) = delete;

    /* Return methods
     * Use these when returning from a TCL command implementation.
     */

    /** 
     * @brief Throw an error from a TCL command.
     */
    inline int _ret_error(TclCommandError&& error) {
        this->cmd_status = e_TclCommandStatus::TCL_CMD_FAIL;
        this->string = std::move(error);
        return TCL_ERROR;
    }

    /** 
     * @brief Return from a TCL command with no errors.
     */
    inline int _ret_ok() {
        this->cmd_status = e_TclCommandStatus::TCL_CMD_SUCCESS;
        return TCL_OK;
    }

    /** 
     * @brief Return a string from a TCL command with no errors.
     */
    inline int _ret_string(std::string&& string_) {
        this->cmd_status = e_TclCommandStatus::TCL_CMD_SUCCESS_STRING;
        this->string = std::move(string_);
        return TCL_OK;
    }

    /**
     * @brief Move an object to TCL-managed context and return it in TCL code.
     * @param T registered custom TCL type
     * @param this_ pointer to the concrete client. Note that it might be different than `this` within context of base class.
     * @param object_data object to be moved into TCL-managed context
     */
    template<typename T, class = typename std::enable_if<std::is_move_constructible<T>::value>::type>
    int _ret_obj(void* this_, const T&& object_data) {
        Tcl_Obj* obj = Tcl_MoveCppObject<T>(this_, object_data);

        this->object = obj;
        this->cmd_status = e_TclCommandStatus::TCL_CMD_SUCCESS_OBJECT;

        return TCL_OK;
    }

    template<typename T, typename Container>
    TclList<T> _list(void* this_, Container&& elements) {
        return TclList<T>(this->_interp, reinterpret_cast<void*>(this_), std::move(elements));
    }

    template<typename T>
    int _ret_list(void* this_, TclList<T>& list) {
        this->object = list.tcl_obj();
        this->cmd_status = e_TclCommandStatus::TCL_CMD_SUCCESS_LIST;

        return TCL_OK;
    }

  private:
    friend class TclCtx;
    template<typename T>
    friend TclList<T> tcl_obj_getlist(TclClient* client, Tcl_Obj* obj);

    Tcl_Interp* _interp;
};

template<typename T>
static TclList<T> tcl_obj_getlist(TclClient* client, Tcl_Obj* obj) {
    return TclList<T>(client->_interp, obj);
}

/* Default implementations for handling TCL-managed C++ objects */
template<typename T, class = typename std::enable_if<std::is_destructible<T>::value>::type>
static void tcl_obj_free(Tcl_Obj* obj) {
    T* obj_data = tcl_obj_getptr<T>(obj);
    obj_data->~T();
    Tcl_Free(reinterpret_cast<char*>(obj_data));
}
void tcl_obj_dup(Tcl_Obj* src, Tcl_Obj* dst);
int tcl_set_from_none(Tcl_Interp* tcl_interp, Tcl_Obj* obj);

/**
 * @brief set TCL object's string representation
 */
void tcl_set_obj_string(Tcl_Obj* obj, const std::string& str);

/**
 * @brief Provide a TCL runtime.
 * Allows extending TCL language with new types which are associated with C++ types and new methods provided by objects of TclClient-derived classes.
 * 
 * `TclCtx` binds a TCL context to the lifetime of a C++ object, so once TclCtx::~TclCtx gets called, TCL context will be cleaned-up.
 * It is illegal to call any method of `TclCtx` othar than its destructor after destroying a bound client.
 */
class TclCtx {
  public:
    /** 
     * @brief Create and initialize context, then run a closure with it.
     * This is done to limit TclCtx lifetime.
     */
    template<typename T>
    static T with_ctx(std::function<T(TclCtx& ctx)> fun) {
        TclCtx ctx;
        ctx._init();
        fun(ctx);
    }

    /**
     * @brief Read and interpret a TCL script within this context.
     */
    void read_tcl(std::istream& tcl_stream);

    /**
     * @brief Add a client class that provides methods to interface with a context managed by it
     */
    template<typename C>
    void add_tcl_client(C& client) {
        auto* client_container = new TclClientContainer<C>(client);
        auto register_command = [&](const char* name, int (C::*method)(int objc, Tcl_Obj* const objvp[])) {
            client_container->methods.push_front(TclMethodDispatch<C>(method, client));
            auto& md = client_container->methods.front();
            Tcl_CreateObjCommand(this->_tcl_interp, name, TclCtx::_tcl_do_method, static_cast<TclMethodDispatchBase*>(&md), nullptr);
        };

        client._interp = this->_tcl_interp;

        client.register_methods(register_command);

        this->_tcl_clients.push_front(std::unique_ptr<TclClientContainerBase>(static_cast<TclClientContainerBase*>(client_container)));
    }

    /**
     * @brief Add a custom type to TCL.
     * 
     * The type has to be moveable in order to allow passing it between Tcl-managed cotext and C++ code.
     * Use REGISTER_TCL_TYPE macro to associate type T with a static struct instance describing the Tcl type.
     */
    template<typename T, class = typename std::enable_if<std::is_destructible<T>::value && std::is_move_constructible<T>::value>::type>
    inline void add_tcl_type() {
        Tcl_RegisterObjType(tcl_type_of<T>());
    }

  protected:
    TclCtx();

    ~TclCtx();

    /**
     * @brief Initialize a newly created instance of TclCtx. It's illegal to call any other method before calling this one.
     */
    void _init();

    /**
     * @brief Tcl by default will interpret bus indices as calls to commands
     *        (eg. in[0] gets interpreted as call to command `i` with argument being a
     *        result of calling `0`). This overrides this behaviour.
     * 
     * TODO: This is SDC/XDC-specific and should be handled somewhere else.
     */
    /* int _fix_port_indexing(); */

    /**
     * @brief Provide an interface to call a client method.
     */
    struct TclMethodDispatchBase {
        inline TclMethodDispatchBase(TclClient& client_)
            : client(client_) {}
        virtual ~TclMethodDispatchBase() {}
        virtual int do_method(int objc, Tcl_Obj* const objvp[]) = 0;
        TclClient& client;
    };

    /**
     * @brief Wrap a call-by-PTM (pointer-to-method) of a concrete TclClient-derived class as a virtual function of TclMethodDispatchBase.
     * This allows us to avoid making methods defined within TclClient-derived virtual methods of TclClient.
     */
    template<typename C>
    struct TclMethodDispatch : TclMethodDispatchBase {
        typedef int (C::*TclMethod)(int objc, Tcl_Obj* const objvp[]);
        inline TclMethodDispatch(TclMethod method_, TclClient& client_)
            : TclMethodDispatchBase(client_)
            , method(method_) {}
        virtual ~TclMethodDispatch() = default;

        virtual int do_method(int objc, Tcl_Obj* const objvp[]) {
            auto& casted_client = static_cast<C&>(this->client);
            return (casted_client.*(this->method))(objc, objvp);
        }

        TclMethod method;
    };

    struct TclClientContainerBase {};

    /**
     * @brief Store a reference to a client along with method dispatch list associated with that client.
     */
    template<typename C>
    struct TclClientContainer : TclClientContainerBase {
        inline TclClientContainer(C& client_)
            : client(client_) {}
        C& client;
        /* Use list to guarantee the validity of references */
        std::list<TclMethodDispatch<C>> methods;
    };

    static int _tcl_do_method(
        ClientData cd,
        Tcl_Interp* tcl_interp,
        int objc,
        Tcl_Obj* const objvp[]);

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

#define TCL_TYPE_OF(cxx_type) cxx_type##_tcl_t

/**
 * @brief Declare Tcl type associated with a C++ type
 * @param cxx_type C++ type name.
 */
#define DECLARE_TCL_TYPE(cxx_type)                         \
    extern const Tcl_ObjType TCL_TYPE_OF(cxx_type);        \
    template<>                                             \
    constexpr const Tcl_ObjType* tcl_type_of<cxx_type>() { \
        return &TCL_TYPE_OF(cxx_type);                     \
    }

/**
 * @brief Associate a C++ struct/class with a `Tcl_ObjType`.
 * @param cxx_type C++ type name.
 * 
 * Foollow it with a body of updateStringProc and close it with END_REGISTER_TCL_TYPE:
 * REGISTER_TCL_TYPE_W_STR_UPDATE(CppType)(Tcl_Obj* obj) { ...body... } END_REGISTER_TCL_TYPE;
 */
#define REGISTER_TCL_TYPE_W_STR_UPDATE(cxx_type)             \
    const Tcl_ObjType TCL_TYPE_OF(cxx_type) {                \
        /* .name = */ #cxx_type,                             \
            /* .freeIntRepProc = */ tcl_obj_free <cxx_type>, \
            /* .dupIntRepProc = */ tcl_obj_dup,              \
            /* .updateStringProc = */[]

#define END_REGISTER_TCL_TYPE                     \
    ,                                             \
        /* .setFromAnyProc = */ tcl_set_from_none \
    }

#endif