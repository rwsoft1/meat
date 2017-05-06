/*                                                                  -*- c++ -*-
 * Copyright (c) 2017 Ron R Wills <ron.rwsoft@gmail.com>
 *
 * This file is part of Meat.
 *
 * Meat is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Meat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Meat.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file */

#include <meat/memory.h>
#include <meat/types.h>

#include <map>
#include <string>
#include <stdexcept>
#include <vector>
#include <deque>
#include <iostream>

#ifndef _MEAT_H
#define _MEAT_H

#if defined(__WIN32__)
#   define DLL_EXPORT __declspec(dllexport)
#   define DLL_IMPORT __declspec(dllimport)
#elif defined(__linux__)
#   define DLL_EXPORT __attribute__((visibility("default")))
#   define DLL_IMPORT
#   if __GNUC__ > 4
#       define DLL_LOCAL __attribute__((visibility("hidden")))
#   else
#       define DLL_LOCAL
#   endif
#else
#   error("Don't know how to export shared object libraries")
#endif

#ifdef MEAT_BUILD
#define DECLSPEC DLL_EXPORT
#else
#define DECLSPEC DLL_IMPORT
#endif

/** @ns */
namespace meat {

	class DECLSPEC Object;
	class DECLSPEC Class;
	class DECLSPEC Context;
	class DECLSPEC BlockContext;
	class DECLSPEC Exception;
	namespace data {
		class Archive; // Found in datastore.h
	};

	/** A reference counter pointer to Script Objects and classes.
	 */
	typedef memory::reference<Object> Reference;

#define CONTEXT(ref) ((meat::Context &)(*(ref)))
#define CLASS(ref) ((meat::Class &)(*(ref)))

	/** Initializes the scripting engine.
	 */
	DECLSPEC void initialize(int argc, const char *argv[]);

	/** Creates a new message context to an Object.
	 * @param Object The Object to send the message to.
	 * @param hash_id The hashed id of the method name.
	 * @param context The calling context.
	 * @return A new context to be executed by meat::execute().
	 * @see meat::execute()
	 * @see hash()
	 */
	DECLSPEC Reference message(Reference &Object,
														 uint32_t hash_id,
														 Reference &context);

	DECLSPEC Reference message(Reference &Object,
														 const char *method,
														 Reference &context);

	DECLSPEC Reference message_super(Reference &Object,
																	 uint32_t hash_id,
																	 Reference &context);

	/** Executes a context that has been created by meat::message().
	 * @param context The context created by message.
	 * @return The results of the message.
	 * @see meat::message()
	 */
	DECLSPEC Reference execute(Reference &context);

#ifdef DEBUG
	std::ostream &operator <<(std::ostream &out, Class &cls);
#endif /* DEBUG */

	/****************************************************************************
	 * Base Object class for all data within the scripting game engine.
	 */
	class DECLSPEC Object {
	public:
		/** Creates a new Object of the given class type.
		 * @param type A Reference to the class for the new Object.
		 */
		explicit Object(Reference &type);

		/**
		 */
		Object(Reference &type, uint8_t properties);

		/** Creates a new integer Object.
		 * @param value The value to initialize the Object with.
		 */
		explicit Object(int32_t value);

		/** Create a new meat number Object from a float_t value.
		 */
		explicit Object(float_t value);

		/** Create a new script boolean Object. This constructor should never be
		 * used exception to initialize the actual true and false Objects and the
		 * True() and False() functions should be used to get references to these
		 * Objects.
		 *
		 * @see True()
		 * @see False()
		 */
		explicit Object(bool value);
		explicit Object(const char *value);
		Object(const char *value, const char *value2);

		/** Destroy and clean up the Object.
		 */
		virtual ~Object () throw ();

		/** Returns a reference to the class type for the Object.
		 */
		Reference get_type() const { return o_type; };

		/** Tests Object if it is of type class.
		 */
		bool is_type(Reference &cls) const;
		bool is_type(const char *class_name) const;

		virtual bool is_class() const { return false; };

		virtual bool is_Object() const { return true; };

		/** Serialize the Object data to a data store.
		 * @param store The store where Object is getting serialized to. This is
		 *              given to the serialize method for the purpose of added
		 *              the Objects properties to the store's index.
		 * @param data_stream The ostream to write the Objects native data.
		 */
		virtual void serialize(data::Archive &store,
													 std::ostream &data_stream) const;

		/** Populates the Object with native data from a data store. Note that the
		 * properties are handled by the data store and only native data should be
		 * read in here.
		 */
		virtual void unserialize(data::Archive &store,
														 std::istream &data_stream);

		/** Returns the number of properties the Object contains.
		 * @return Return the number of properties.
		 */
		uint8_t get_num_of_props() const { return num_of_props; }
    virtual Reference &property(uint8_t index);
    virtual Reference &property(uint8_t index) const;

		virtual bool operator == (const char *value) const;

		virtual operator Class &();
		virtual operator Context &();

		/** Converts the Object to an integer if possible.
		 * @throws Exception If the Object cannot be converted to an integer.
		 * @return Returns an integer.
		 */
    virtual int32_t to_int() const;
    virtual float_t to_float() const;
		virtual const char *to_string() const;

		friend void meat::initialize(int argc, const char *argv[]);

	private:
		meat::Reference o_type;

		typedef enum {
			PROPSONLY = 0x00,
			BOOLEAN   = 0xfc,
			INTEGER   = 0xfd,
			FLOAT     = 0xfe,
			STRING    = 0xff,
		} data_type_t;

		uint8_t num_of_props;
		meat::Reference *properties;

		data_type_t data_type;
		union {
			int32_t i;
			float_t f;
			bool b;
			char *s;
		} data;
	};

	/****************************************************************************
   * meat Class
	 */

	/** Script Object class primative method pointer.
	 * @param context The context the method is currently executing with in.
	 * @return The results from the primative method.
	 * @see meat::Context
	 * @see meat::Class::VTable
	 */
	typedef Reference (*method_ptr_t)(Reference &context);

	/** Function type for creating new Objects from the class.
	 */
	typedef Reference (*constructor_t)(Reference &cls, uint8_t properties);

#define VTM_END      (0x00)
#define VTM_BYTECODE (0x01) // Vtable entry points to bytecode
#define VTM_NATIVE   (0x02) // Vtable entry points to a native function.
#define VTM_SUPER    (0x04) // Vtable entry needs to be synced from super.

	typedef struct _vtable_entry_s {
		uint32_t hash_id; // All entries must be sorted by this id.
		uint32_t class_id;
		uint8_t flags;
		uint8_t locals;   // This includes parameters with local variables
		union {
			meat::method_ptr_t pointer;
			uint16_t offset;
		} method;
	} vtable_entry_t;

  typedef enum {STATIC, DYNAMIC, COPY} alloc_t;

	/**
	 */
	class DECLSPEC Class : public Object {
	public:
		/** Creates a new class Object.
     */
		Class (const char *parent, uint8_t obj_props = 0);
		Class (const char *parent, uint8_t cls_props, uint8_t obj_props);
		Class (Reference &parent, uint8_t obj_props = 0);
		Class (Reference &parent, uint8_t cls_props, uint8_t obj_props);

		virtual ~Class () throw ();

		/** Get the meat super class for this class.
     * @return A Reference to the class's super class.
     */
		Reference get_super() const;

		virtual uint8_t get_obj_properties() const;

		virtual bool is_class() const { return true; };
		virtual bool is_object() const { return false; };

		/** Creates a new Object of this class type. To create the actual Object
     * the constructor function, set by set_constructor(), is called to
     * create the actual Object.
     * @return A reference to the new Object.
     * @see set_constructor
		 */
		Reference new_object();

    /** Gets the internal 32bit hash id for the class.
     */
		uint32_t get_hash_id() const { return hash_id; };

    /** Set the constructor function. The constructor is used by the method
     * new_Object() to create the actual internal Object. If a constructor
     * function is not set the parent meat classes will be checked for a
     * constuctor function. Most of the time the default Object constructor
     * is enough unless a unique C++ class was created for the Object.
     *
     * @param constructor Pointer to the contructor function.
     * @see new_Object()
     */
		void set_constructor(constructor_t constructor);
		void set_vtable(uint8_t entries, vtable_entry_t table[],
										alloc_t table_alloc);
		void set_class_vtable(uint8_t entries, vtable_entry_t table[],
													alloc_t table_alloc);
		void set_bytecode(uint16_t size, uint8_t *code, alloc_t code_alloc);

		const vtable_entry_t *get_vtable(uint8_t &count) const;
		const vtable_entry_t *get_class_vtable(uint8_t &count) const;
		const uint8_t *get_bytecode() const;

		virtual operator Class &() { return *this; };

		virtual void write(std::ostream &lib_file) const;
		static Class *import(std::istream &lib_file);

    //@{
		/** Register a class with the give id name.
		 * @param cls The class being registered.
		 * @param id The name of the class used in the Script engine.
     * @param replace By default an error is raised if an existing recorded
     *        class already exists. If replace is set to true the this is
     *        overrided. This should only be used internally.
		 */
		static void record(Class *cls, const char *id, bool replace = false);
		static void record(Reference &cls, const char *id, bool replace = false);
    static void record(Reference &cls, bool replace = false);
    //@}

		void relink();

		static void unrecord(Reference &cls);

		/** Resolves a class that has been registered with record.
		 * @param id The text name of the class.
		 * @see record()
		 * @exception Exception If the class cannot be found.
		 */
		static Reference &resolve(const char *id);

		/** Resolves a class that has been registered with record.
		 * @param hash_id The integer hash id of the class.
		 * @see record()
		 * @see hash()
		 * @exception Exception If the class cannot be found.
		 */
		static Reference &resolve(uint32_t hash_id);

    /**
     */
		static bool have_class(const char *id);

		/** Returns a reference to the class. This uses the resolve method
		 * to find the reference so the class @b must be registered with record()
		 * before this is called.
		 * @see resolve()
		 * @see record()
		 */
		operator Reference &() const;

#ifdef DEBUG
		friend std::ostream &operator <<(std::ostream &out, Class &cls);
#endif /* DEBUG */
		friend void meat::initialize(int argc, const char *argv[]);
		friend Reference message(Reference &Object,
														 uint32_t hash_id,
														 Reference &context);
		friend Reference message_super(Reference &Object,
																	 uint32_t hash_id,
																	 Reference &context);
		friend Reference execute(Reference &context);

	private:
    /** The virtual method table for class Objects.
		 */
		class VTable {
		public:
			VTable();
			virtual ~VTable() throw();

			void set_vtable(uint8_t entries, vtable_entry_t table[],
                      alloc_t table_alloc);
			void set_class_vtable(uint8_t entries, vtable_entry_t table[],
														alloc_t table_alloc);
			void set(constructor_t constructor_func);

			void link(Class &super);

      /** Returns the virtual table entry if the method was found.
			 */
			vtable_entry_t *find(uint32_t hash_id);
			const vtable_entry_t *find(uint32_t hash_id) const;
			vtable_entry_t *class_find(uint32_t hash_id);
			const vtable_entry_t *class_find(uint32_t hash_id) const;

      void write(std::ostream &lib_file) const;
			void read(std::istream &lib_file);

      friend class Class;
			friend Reference message(Reference &Object,
															 uint32_t hash_id,
															 Reference &context);
			friend Reference message_super(Reference &Object,
																		 uint32_t hash_id,
																		 Reference &context);
		private:
			// Object virtual table entries
			uint8_t no_entries;
			vtable_entry_t *entries;
			bool e_is_static;

			// Class virtual table entries.
			uint8_t no_centries;
			vtable_entry_t *centries;
			bool ce_is_static;

			constructor_t constructor;
		};

		class ByteCode {
		public:
			ByteCode();
			virtual ~ByteCode() throw();

			void write(std::ostream &lib_file) const;

			void read(std::istream &lib_file);

			void set(unsigned int size, uint8_t *code, alloc_t code_alloc);

			const uint8_t *get() const;

		private:
			uint16_t size;
			uint8_t *code;
			bool is_static;
		};

		const vtable_entry_t *find(uint32_t hash_id) const;
		const vtable_entry_t *class_find(uint32_t hash_id) const;

		uint32_t hash_id;
#ifdef DEBUG
		std::string name;
#endif /* DEBUG */
		Reference super;
		uint8_t obj_properties;

		VTable vtable;
		ByteCode bytecode;
	};

	/****************************************************************************
	 */

	/** When a message is created for a given object, a context is created for
	 * the execution of the message. The context contains all the local
	 * variables needs for the answering method.
	 *  For bytecode methods the context also contains a reference to the
	 * method's bytecode and a pointer to the current bytecode being
	 * interpreted. Native methods just have a pointer to the method.
	 *
	 * How the local variables are setup:
	 * - self - A reference to the object being messaged.
	 * - class - A reference to the class object of the method answering the
	 *           message.
	 * - context - A weak reference to itself. Used so the answering method has
	 *             access to the context.
	 * - null - A reference to the null object.
	 * - locals required for the method.
	 */
	class DECLSPEC Context : public Object {
	public:
		/** Create a new context with no calling context.
		 * @locals The number of local variables to allocate.
		 */
		explicit Context(uint8_t locals);
		Context(Reference &context, uint8_t locals);
		Context(Reference &cls, Reference &context, uint8_t locals);
		virtual ~Context() throw();

		virtual Reference get_self() const;
		void reset_self(Reference &new_self);

		virtual Reference get_class() const { return locals[1]; }
		virtual Reference get_context() const { return locals[2]; }
		virtual Reference get_super() const {
      return CLASS(get_class()).get_super();
    };

		/** Get the calling context.
		 */
		virtual Reference get_uplevel() const;
		virtual void set_uplevel(Reference &context);

		virtual Reference get_param(uint8_t index) const;

		virtual void set_param(uint8_t index, Reference &value);

		/**
		 */
		virtual void set_local(uint8_t index, Reference &value);

		/** Get a local variable from the context. If the index is out of range
		 * then an meat::Exception is raised.
		 * @param index The index to the locals variable desired.
		 * @return A reference to the local variable.
		 * @see get_num_of_locals()
		 */
		virtual Reference get_local(uint8_t index) const;

		/** Returns the number of local variables the context contains. This is
		 * set when the context is created and cannot be changed.
		 * @return The number of local variables.
		 */
		virtual uint8_t get_num_of_locals() const { return num_of_locals; };

		uint16_t get_ip() const { return ip; };
		void set_ip(uint16_t offset) { ip = offset; };

		void set_result_index(uint8_t local_parent_index);
		void set_result(Reference &value);
		Reference get_result() const;

		virtual bool is_done() { return done; };
		virtual void finish() { done = true; };
		virtual void unfinish() { done = false; };

		friend class Class;
		friend class BlockContext;
		friend Reference message(Reference &Object,
														 uint32_t hash_id,
														 Reference &context);
		friend Reference message_super(Reference &Object,
																	 uint32_t hash_id,
																	 Reference &context);
		friend Reference execute(Reference &context);

	protected:
		/** The context that this context was created in. If this is Null then
		 * the context is the top level context of the program.
		 */
		Reference up_level;
		Reference result;

		typedef enum {
			NOMETHOD  = 0x00,
			PRIMATIVE = 0x01,
			BYTECODE  = 0x02
		} method_t;

		method_t flags;
		meat::method_ptr_t pointer;
		uint16_t ip;

	private:
		uint8_t num_of_locals;
		Reference *locals;
		uint8_t result_index;

		bool done;
	};

	/****************************************************************************
	 */

	/**
	 * @todo I really need to document how this guy works.
	 */
	class DECLSPEC BlockContext : public Context {
	public:
		BlockContext(uint8_t locals);
		BlockContext(Reference &context, uint8_t locals, uint16_t ip);
		virtual ~BlockContext() throw();

		virtual Reference get_self() const {
			return CONTEXT(context).get_self();
		}

		virtual Reference get_class() const {
			return CONTEXT(context).get_class();
		}
		virtual Reference get_context() const {
			return CONTEXT(context).get_context();
		}
		virtual Reference get_super() const {
			return CLASS(get_class()).get_super();
		}

		virtual Reference get_param(uint8_t index) const {
			return locals[index];
		};

		virtual void set_param(uint8_t index, Reference &value) {
			locals[index] = value;
		};

		/** Set a local variable with the given value. The index for the local
		 * variable starts in the containing context then appending the block
		 * context's variables after.
		 * @param index The local index of the variable.
		 * @param value The value to set the local variable to.
		 * @throw Exception If the index is out of range.
		 */
		virtual void set_local(uint8_t index, Reference &value);
		virtual Reference get_local(uint8_t index) const;

		/** Returns the number of locals in this context and in the containing
		 * context.
		 * @return uint8_t Total number of local variables.
		 */
		virtual uint8_t get_num_of_locals() const;

		Reference get_operating_context() { return context; }

		virtual bool is_done();

	private:
		Reference context;
	};

	/****************************************************************************
	 */

	class DECLSPEC Exception : public std::exception, public Object {
	public:
		Exception();
		Exception(const Exception &other);
		Exception(Reference &cls, uint8_t properties);
		Exception(const char *message);
		Exception(std::string message);
		Exception(const char *message, Reference &context);
		Exception(std::string message, Reference &context);
		Exception(Reference &message, Reference &context);
		virtual ~Exception() throw() {}

		virtual const char* what() const throw();
	};

	/****************************************************************************
	 */

	class DECLSPEC List : private std::deque<Reference>, public Object {
	public:
		List();
		List(Reference &cls, uint8_t properties);
		virtual ~List() throw() {}

		using std::deque<Reference>::push_back;
		using std::deque<Reference>::pop_back;
		using std::deque<Reference>::clear;
		using std::deque<Reference>::at;
		//using std::deque<Reference>::operator[];
		using std::deque<Reference>::insert;
		using std::deque<Reference>::erase;
		using std::deque<Reference>::back;
		using std::deque<Reference>::size;
		using std::deque<Reference>::empty;
		using std::deque<Reference>::begin;
		using std::deque<Reference>::end;

		using std::deque<Reference>::iterator;
		using std::deque<Reference>::const_iterator;

		virtual void serialize(data::Archive &store,
													 std::ostream &data_stream) const;
		virtual void unserialize(data::Archive &store, std::istream &data_stream);
	};

	/****************************************************************************
	 */

	class DECLSPEC obj_less {
	public:
		bool operator()(const Reference &first, const Reference &second) const;
	};

	class DECLSPEC Index : private std::map<Reference, Reference, obj_less>,
												 public Object {
	public:
		Index();
		Index(Reference &cls, uint8_t properties);
		virtual ~Index() throw() {}

		using std::map<Reference, Reference, obj_less>::clear;
		using std::map<Reference, Reference, obj_less>::find;
		using std::map<Reference, Reference, obj_less>::operator[];
		using std::map<Reference, Reference, obj_less>::insert;
		using std::map<Reference, Reference, obj_less>::erase;
		//using std::map<Reference, Reference, obj_less>::back;
		using std::map<Reference, Reference, obj_less>::size;
		using std::map<Reference, Reference, obj_less>::empty;
		using std::map<Reference, Reference, obj_less>::begin;
		using std::map<Reference, Reference, obj_less>::end;

		using std::map<Reference, Reference, obj_less>::iterator;
		using std::map<Reference, Reference, obj_less>::const_iterator;

		virtual void serialize(data::Archive &store,
													 std::ostream &data_stream) const;
		virtual void unserialize(data::Archive &store, std::istream &data_stream);
	};

	/** A quick way of resolving a reference to the Class class Object.
	 * @param initializing Used internally during initialization, not to be
	 *                     used otherwise.
	 * @return A reference to the Class class Object.
	 */
	DECLSPEC Reference &ClassClass(bool initializing = false);

	/** A quick way of resolving a Reference to the True Object.
	 * @return A Reference to the True Object.
	 */
	DECLSPEC Reference &True();

	DECLSPEC Reference &False();

	DECLSPEC Reference &Null();

};

#endif /* _MEAT_H */
