/*                                                                  -*- c++ -*-
 * The Library Compiler
 *
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

#include <meat/compiler.h>
#include <meat/utilities.h>

#include "ast.h"

#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>
#include <iterator>

#ifdef DEBUG
#include <iostream>
#endif

#ifdef TESTING
#include <testing.h>
#endif

using namespace meat;

struct _c_vtable_entry_s {
  meat::uint32_t hash_id;
  std::string str_hash;
	std::string str_class_hash;
  std::string flags;
  meat::uint8_t locals;
  std::string func_name;
};

/** Converts an integer value to a string.
 */
static std::string to_string(unsigned int value) {
  std::ostringstream convert;
  convert << value;
  return convert.str();
}

/** Changes the gitgo function name to a name more suitable for c++.
 */
static std::string cook_c_name(const char *value) {
  std::string method_name(value);
  for (unsigned int c = 0; c < method_name.size(); c++) {
    if (method_name[c] == ':' || method_name[c] == '.')
      method_name[c] = '_';
  }
  return method_name;
}

static bool c_vtable_comp(struct _c_vtable_entry_s left,
                          struct _c_vtable_entry_s right) {
  return left.hash_id < right.hash_id;
}

static bool vtable_comp(struct _vtable_entry_s left,
                        struct _vtable_entry_s right) {
  return left.hash_id < right.hash_id;
}

/******************************************************************************
 * Class VtableBuilder
 */

template <class Ty>
class VtableBuilder {
private:
	std::vector<Ty> vtable;

public:
	typedef typename std::vector<Ty>::const_iterator const_iterator;
	typedef typename std::vector<Ty>::iterator iterator;

	VtableBuilder() {}

	void set(const struct _vtable_entry_s *table, uint8_t count) {
		static Ty foo;
		vtable.clear();
		for (unsigned int c = 0; c < count; ++c) {
			vtable.push_back(foo);
			vtable.back().hash_id = table[c].hash_id;
			vtable.back().flags = VTM_SUPER;
		}
	}

	void add_entry(const Ty &entry) {
		iterator it = vtable.begin();
		for (; it != vtable.end(); ++it) {
			if (it->hash_id == entry.hash_id) {
				vtable.erase(it);
				break;
			}
		}

		vtable.push_back(entry);
	}

	uint8_t size() { return vtable.size(); }

	const_iterator begin() const { return vtable.begin(); }
	iterator begin() { return vtable.begin(); }
	const_iterator end() const { return vtable.end(); }
	iterator end() { return vtable.end(); }

	void sort() {
		std::sort(vtable.begin(), vtable.end(), vtable_comp);
	}

	struct _vtable_entry_s *entries() {
		return &vtable[0];
	}

	void clear() { vtable.clear(); }
};

/******************************************************************************
 * meat::compiler::LibraryBuilder Class
 */

compiler::LibraryBuilder::LibraryBuilder(const char *name)
  : Object(Class::resolve("Compiler.Library"), 2), to_cpp(false) {

  library = data::Library::create(name);

  this->property(0) = new Object(name);
	this->property(1) = new List();
}

compiler::LibraryBuilder::~LibraryBuilder() throw() {
  data::Library::unload(library->get_name());
}

/*********************************************
 * meat::compiler::LibraryBuilder::new_class *
 *********************************************/

Reference compiler::LibraryBuilder::new_class(Reference &super,
                                              const char *name) {
  Reference cls = new ClassBuilder(*this, super, name);
	Reference &classes = this->property(1);
	(dynamic_cast<meat::List &>(*classes)).push_back(cls);
  return cls;
}

/*************************
 * LibraryBuilder::write *
 *************************/

void compiler::LibraryBuilder::write() {
  if (is_cpp()) {
    std::string cppcode;

    cppcode  = "/*************************************************************"
               "*****************\n";
    cppcode += " * Auto-generated by the meat compiler\n";
    cppcode += " */\n\n";
    cppcode += "#include <meat.h>\n";
    cppcode += "#include <meat/datastore.h>\n";
    cppcode += data::Library::include();

    cppcode += "\n#define null (meat::Null())\n\n";

    List::const_iterator cit;

    // Create all the class methods and vtables.
    List &classes = (List &)(*(this->property(1)));
    for (cit = classes.begin(); cit != classes.end(); cit++) {
      cppcode += ((ClassBuilder &)(*(*cit))).cpp_methods();
    }

    // Declare the init function using C conventions.
    cppcode += "// We need to declare init_" +
      std::string(library->get_name()) + " as a C function.\n";
    cppcode += "extern \"C\" {\n";
    cppcode += "  DECLSPEC void init_" + std::string(library->get_name()) +
      "(data::Library &library);\n}\n\n";

    // Generate the init function.
    cppcode += "void init_" + std::string(library->get_name()) +
      "(data::Library &library) {\n  meat::Class *cls;\n";

    // Create all the class methods and vtables.
    for (cit = classes.begin(); cit != classes.end(); cit++) {
      cppcode += ((ClassBuilder &)(*(*cit))).cpp_new_class();
    }

    cppcode += "}\n";

		// Now write the code to the file stream.
    std::ofstream cpp_file;
    cpp_file.open((std::string(library->get_name()) + ".cpp").c_str());
    if (cpp_file.is_open()) {
      cpp_file << cppcode << std::flush;
      cpp_file.close();
    }

  } else {
    library->write();
  }
}

/***************************
 * LibraryBuilder::command *
 ***************************/

void compiler::LibraryBuilder::command(Tokenizer &tokens) {
  Reference obj;
  std::string object, message;

  /* Determine the object to send the message to. */
  switch (tokens[0].type()) {
  case Token::WORD: {
    object = (std::string &)(tokens[0]);
    obj = Class::resolve(((const std::string &)tokens[0]).c_str());
    break;
  }
  default:
    throw Exception("Syntax error: Unable to resolve class " +
      (std::string &)tokens[0]);
  }

  if (tokens.count() == 2) {
    /* This is an object message with no parameters. */

    // Get the object.
    if (tokens[0].is_type(Token::WORD)) {
      object = (std::string &)(tokens[0]);
    }

    // Get the message.
    if (tokens[1].is_type(Token::WORD)) {
      message = (std::string &)(tokens[1]);
    }

#ifdef DEBUG
    std::cout << "DEBUG: message \"" << message << "\"" << std::endl;
#endif

    // Build the context and execute it.
    context = meat::message(obj, message.c_str(), context);
    result = meat::execute(context);
    context = ((Context &)(*context)).get_messenger();

  } else if (tokens.count() > 2 && tokens.count() % 2 == 1) {
    /* A message with parameters. */

    // Build the message name.
    for (unsigned int c = 1; c < tokens.count(); c += 2) {
      message += (std::string &)(tokens[c]);
    }

#ifdef DEBUG
    std::cout << "DEBUG: message \"" << message << "\"" << std::endl;
#endif

    // Create the context for the new message.
    context = meat::message(obj, message.c_str(), context);

    // Add the parameters to the context.
    uint8_t param;
    uint32_t c;
    for (c = 2, param = 0;
         c < tokens.count();
         c += 2, param++) {
      switch (tokens[c].type()) {
      case Token::WORD:      // Simple object value parameters.
      case Token::BLOCK:
      case Token::SUBST_STRING:
      case Token::LITRL_STRING: {
        Reference text(new Object(((const std::string &)(tokens[c])).c_str()));
        ((Context &)(*context)).set_param(param, text);
        break;
      }
      case Token::COMMAND: { // Parameter that comes from a command result.
        Tokenizer cmd_parser;
        cmd_parser.parse(tokens[0]);
        command(cmd_parser);
        ((Context &)(*context)).set_param(param, result);
        break;
      }
      default:
        throw meat::Exception("Syntax error");
      }
    }

    // Execute the new context.
    result = meat::execute(context);
    context = ((Context &)(*context)).get_messenger();

  } else {
    throw Exception("Syntax error");
  }

#ifdef DEBUG
  //std::cout << "DEBUG:  clear " << object << " " << message << std::endl;
#endif /* DEBUG */
  tokens.clear();
}

/**************************
 * LibraryBuilder::is_cpp *
 **************************/

bool compiler::LibraryBuilder::is_cpp() const {
  List::const_iterator mit;

  // Check the methods to see if any of them are cpp methods.
  List &classes = (List &)(*(this->property(1)));
  for (mit = classes.begin(); mit != classes.end(); mit++) {
    if (((ClassBuilder &)(*(*(mit)))).is_cpp()) {
      return true;
    }
  }
  return false;
}

/******************************************************************************
 * meat::compiler::ClassBuilder Class
 */

/******************************
 * ClassBuilder::ClassBuilder *
 ******************************/

meat::compiler::ClassBuilder::ClassBuilder(LibraryBuilder &library,
                                            Reference &super,
                                            const char *cls_name)
  : Object(Class::resolve("Compiler.Class"), 6), library(library) {
  this->super = super;
  cpp_bytecode = 0;

  this->property(0) = new Object(cls_name);
	this->property(1) = new List();  // Properties
  this->property(2) = new List();  // Class properties
  this->property(3) = new Index(); // Methods
  this->property(4) = new Index(); // Class methods
  //this->property(5, meat::Null());
}

/******************************
 * ClassBuilder::obj_property *
 ******************************/

meat::uint8_t
meat::compiler::ClassBuilder::obj_property(const std::string &name) {
#ifdef DEBUG
  std::cout << "COMPILE: Adding property " << name << " to class."
            << std::endl;
#endif /* DEBUG */
  List &properties = (List &)(*(this->property(1)));

  for (uint8_t index = 0; index < properties.size(); index++) {
    if (*(properties.at(index)) == name.c_str()) return index;
  }

  properties.push_back(new Object(name.c_str()));
  return properties.size() - 1;
}

/******************************
 * ClassBuilder::cls_property *
 ******************************/

uint8_t compiler::ClassBuilder::cls_property(const std::string &name) {
#ifdef DEBUG
  std::cout << "COMPILE: Adding class property " << name << " to class."
            << std::endl;
#endif /* DEBUG */
  List &cls_properties = (List &)(*(this->property(2)));

  for (uint8_t index = 0; index < cls_properties.size(); index++) {
    if (*(cls_properties.at(index)) == name.c_str()) return index;
  }

  cls_properties.push_back(new Object(name.c_str()));
  return cls_properties.size() - 1;
}

/**********************************
 * ClassBuilder::have_obj_propery *
 **********************************/

int16_t compiler::ClassBuilder::have_obj_property(const std::string &name)
  const {

  List &properties = (List &)(*(this->property(1)));

  for (uint8_t index = 0; index < properties.size(); index++) {
    if (*(properties.at(index)) == name.c_str()) return index;
  }
  return -1;
}

/***********************************
 * ClassBuilder::have_cls_property *
 ***********************************/

int16_t compiler::ClassBuilder::have_cls_property(const std::string &name)
  const {
  List &cls_properties = (List &)(*(this->property(2)));

  for (uint8_t index = 0; index < cls_properties.size(); index++) {
    if (*(cls_properties.at(index)) == name.c_str()) return index;
  }
  return -1;
}

/******************************
 * ClassBuilder::create_class *
 ******************************/

void compiler::ClassBuilder::create_class() const {

#ifdef DEBUG
  std::cout << "COMPILER: Creating class " << this->property(0)->to_string()
            << std::endl;
#endif /* DEBUG */

  List &properties = (List &)(*(this->property(1)));
  List &cls_properties = (List &)(*(this->property(2)));

  meat::Class *cls =
    new meat::Class((Reference &)super,
                     (uint8_t)cls_properties.size(),
                     (uint8_t)properties.size() +
                     CONST_CLASS(super).get_obj_properties());

  std::vector<uint8_t> bytecode;

	uint8_t vt_size;
	const vtable_entry_t *vt;
  Index::const_iterator mit;
	VtableBuilder<struct _vtable_entry_s> vtable;
	VtableBuilder<struct _vtable_entry_s>::iterator vtable_it;

  /* Generate the bytecode for the object methods and record it in the
   * virtual table.
   */
	vt = CONST_CLASS(super).get_vtable(vt_size);
	vtable.set(vt, vt_size);
	for (vtable_it = vtable.begin(); vtable_it != vtable.end(); ++vtable_it)
		vtable_it->flags = VTM_SUPER;
  Index &methods = (Index &)(*(this->property(3)));
  for (mit = methods.begin(); mit != methods.end(); mit++) {
    struct _vtable_entry_s entry;
    uint16_t offset = bytecode.size();
    ((MethodBuilder &)(*(mit->second))).gen_bytecode(bytecode);
    entry.hash_id = hash(mit->first->to_string());
		entry.class_id = hash(this->property(0)->to_string());
    entry.flags = VTM_BYTECODE;
    entry.locals = ((MethodBuilder &)(*(mit->second))).local_count();
    entry.method.offset = offset;
#ifdef DEBUG
		std::cout << "VTABLE: method " << mit->first->to_string()
							<< " bytecode @ " << offset << std::endl;
#endif
		vtable.add_entry(entry);
  }
	std::sort(vtable.begin(), vtable.end(), vtable_comp);
  cls->set_vtable(vtable.size(), vtable.entries(), COPY);
  vtable.clear();

  /* Generate the bytecode for the class methods and record it in the class
   * virtual table.
   */
	vt = CONST_CLASS(super).get_class_vtable(vt_size);
	vtable.set(vt, vt_size);
	for (vtable_it = vtable.begin(); vtable_it != vtable.end(); ++vtable_it)
		vtable_it->flags = VTM_SUPER;
  Index &class_methods = (Index &)(*(this->property(4)));
  for (mit = class_methods.begin(); mit != class_methods.end(); mit++) {
    struct _vtable_entry_s entry;
    uint16_t offset = bytecode.size();
    ((MethodBuilder &)(*(mit->second))).gen_bytecode(bytecode);
    entry.hash_id = hash(mit->first->to_string());
		entry.class_id = hash(this->property(0)->to_string());
    entry.flags = VTM_BYTECODE;
    entry.locals = ((MethodBuilder &)(*(mit->second))).local_count();
    entry.method.offset = offset;
#ifdef DEBUG
		std::cout << "VTABLE: class method " << mit->first->to_string()
							<< " bytecode @ " << offset << std::endl;
#endif
    vtable.add_entry(entry);
  }
	std::sort(vtable.begin(), vtable.end(), vtable_comp);
  cls->set_class_vtable(vtable.size(), vtable.entries(), COPY);
  vtable.clear();

  // Add the bytecode to the class.
  cls->set_bytecode(bytecode.size(), (uint8_t *)&bytecode[0], COPY);

  // Register the new class and add it to the library.
  library.library->add(cls, this->property(0)->to_string());
}

/************************
 * ClassBuilder::is_cpp *
 ************************/

bool meat::compiler::ClassBuilder::is_cpp() const {
  Index::const_iterator mit;

  // Check to see if a c++ constructor is set
  if (!(this->property(5) == meat::Null())) {
    return true;
  }

  // Check the methods to see if any of them are cpp methods.
  Index &methods = (Index &)(*(this->property(3)));
  for (mit = methods.begin(); mit != methods.end(); mit++) {
    if (((MethodBuilder &)(*(mit->second))).is_cpp_method()) {
      return true;
    }
  }

  // Check the class methods to see if any of them are cpp methods.
  Index &class_methods = (Index &)(*(this->property(4)));
  for (mit = class_methods.begin(); mit != class_methods.end(); mit++) {
    if (((MethodBuilder &)(*(mit->second))).is_cpp_method()) {
      return true;
    }
  }
  return false;
}

/*****************************
 * ClassBuilder::cpp_methods *
 *****************************/

std::string meat::compiler::ClassBuilder::cpp_methods() {
  std::string cppcode;
  std::string class_name = cook_c_name(this->property(0)->to_string());
  std::vector<uint8_t> bytecode;

  cppcode += "/***************************************************************"
    "***************\n";
  cppcode += std::string(" * ") + (this->property(0)->to_string()) +
    " Class\n";
  cppcode += " */\n\n";

  if (!(this->property(5) == meat::Null())) {
      cppcode += "static meat::Reference " + class_name + "_constructor(\n" +
        "  meat::Reference &klass,\n  meat::uint8_t properties) {\n";
      cppcode += this->property(5)->to_string();
      cppcode += "\n}\n\n";
  }

  List::const_iterator pit;
  unsigned int c;
  List &obj_props = (List &)(*(this->property(1)));
  List &cls_props = (List &)(*(this->property(2)));

  if (obj_props.size() > 0) {
    for (pit = obj_props.begin(), c = 0; pit != obj_props.end(); pit++, c++) {
        cppcode += std::string("#define ") + (*pit)->to_string() +
          " (self->property(" + ::to_string(c) + "))\n";
    }
    cppcode += "\n";
  }
  if (cls_props.size() > 0) {
    for (pit = cls_props.begin(), c = 0; pit != cls_props.end(); pit++, c++) {
        cppcode += std::string("#define ") + (*pit)->to_string() +
          " (klass->property(" + ::to_string(c) + "))\n";
    }
    cppcode += "\n";
  }

	uint8_t vt_size;
	const vtable_entry_t *vt;
	VtableBuilder<struct _c_vtable_entry_s> vtable;
	VtableBuilder<struct _c_vtable_entry_s>::iterator vtable_it;

  /* Methods */
	vt = CLASS(super).get_vtable(vt_size);
	vtable.set(vt, vt_size);
	for (vtable_it = vtable.begin(); vtable_it != vtable.end(); ++vtable_it) {
		vtable_it->str_hash = itohex(vtable_it->hash_id);
		vtable_it->str_class_hash = "0x00000000";
		vtable_it->flags = "VTM_SUPER";
		vtable_it->func_name = "{.offset = 0}";
	}
  if (method_count() > 0) {
    Index &methods = (Index &)(*(this->property(3)));
    //std::vector<struct _c_vtable_entry_s> vtable;

    Index::const_iterator mit;
    for (mit = methods.begin(); mit != methods.end(); mit++) {
      meat::uint32_t hash_id = hash((*(mit->first)).to_string());
      struct _c_vtable_entry_s entry;

      if (((MethodBuilder &)(*(mit->second))).is_cpp_method()) {
        // Build the virtual table entry.
        entry.hash_id = hash_id;
        entry.str_hash = ((MethodBuilder &)(*(mit->second))).cpp_hash_id();
				entry.str_class_hash = cpp_hash_id();
        entry.flags = "VTM_NATIVE";
        entry.func_name =
          ((MethodBuilder &)(*(mit->second))).cpp_name((class_name +
            "_om_").c_str());

        cppcode += "// method " +
          std::string((*(mit->first)).to_string()) +
          "\n";
        cppcode +=
          ((MethodBuilder &)(*(mit->second))).cpp_method((class_name +
            "_om_").c_str());

        entry.locals = ((MethodBuilder &)(*(mit->second))).local_count();

        vtable.add_entry(entry);
      } else {
        // Build the virtual table entry.
        entry.hash_id = hash_id;
        entry.str_hash = ((MethodBuilder &)(*(mit->second))).cpp_hash_id();
				entry.str_class_hash = cpp_hash_id();
        entry.flags = "VTM_BYTECODE";
        entry.func_name = "{.offset = " + ::to_string(bytecode.size()) + "}";
        ((MethodBuilder &)(*(mit->second))).gen_bytecode(bytecode);
        entry.locals = ((MethodBuilder &)(*(mit->second))).local_count();
        vtable.add_entry(entry);
      }
    }

    // Sort the virtual table according to the hash ids.
    std::sort(vtable.begin(), vtable.end(), c_vtable_comp);
		m_count = vtable.size();

    cppcode += "static meat::vtable_entry_t " +
      class_name +
      "Methods[] = {\n";

    bool first = true;
    std::vector<struct _c_vtable_entry_s>::const_iterator it;
    for (it = vtable.begin(); it != vtable.end(); it++) {
      if (!first) cppcode += ",\n";
      else first = false;

      cppcode += "  {" + it->str_hash + ", " + it->str_class_hash +
				", " + it->flags + ", " + ::to_string(it->locals) + ", " +
        it->func_name + "}"; // XXX Need to deal with the comma.
    }
    cppcode += "\n";

    cppcode += "};\n\n";
  }

  if (obj_props.size() > 0) {
    for (pit = obj_props.begin(); pit != obj_props.end(); pit++) {
      cppcode += std::string("#undef ") + (*pit)->to_string() + "\n";
    }
    cppcode += "\n";
  }

  /* Class methods */
	vt = CLASS(super).get_class_vtable(vt_size);
	vtable.set(vt, vt_size);
	for (vtable_it = vtable.begin(); vtable_it != vtable.end(); ++vtable_it) {
		vtable_it->str_hash = itohex(vtable_it->hash_id);
		vtable_it->str_class_hash = "0x00000000";
		vtable_it->flags = "VTM_SUPER";
		vtable_it->func_name = "{.offset = 0}";
	}
  if (class_method_count() > 0) {
    Index &methods = (Index &)(*(this->property(4)));

    Index::const_iterator mit;
    for (mit = methods.begin(); mit != methods.end(); mit++) {
      meat::uint32_t hash_id = hash((*(mit->first)).to_string());
      struct _c_vtable_entry_s entry;

      if (((MethodBuilder &)(*(mit->second))).is_cpp_method()) {
        // Build the virtual table entry.
        entry.hash_id = hash_id;
        entry.str_hash = ((MethodBuilder &)(*(mit->second))).cpp_hash_id();
				entry.str_class_hash = cpp_hash_id();
        entry.flags = "VTM_NATIVE";
        entry.func_name = ((MethodBuilder &)(*(mit->second))).cpp_name((class_name + "_cm_").c_str());

        cppcode += "// class method " +
          std::string((*(mit->first)).to_string()) +
          "\n";
        cppcode += ((MethodBuilder &)(*(mit->second))).cpp_method((class_name + "_cm_").c_str());

        entry.locals = ((MethodBuilder &)(*(mit->second))).local_count();

        vtable.add_entry(entry);
      } else {
        // Build the virtual table entry.
        entry.hash_id = hash_id;
        entry.str_hash = ((MethodBuilder &)(*(mit->second))).cpp_hash_id();
				entry.str_class_hash = cpp_hash_id();
        entry.flags = "VTM_BYTECODE";
        entry.func_name = "{.offset = " + ::to_string(bytecode.size()) + "}";
        ((MethodBuilder &)(*(mit->second))).gen_bytecode(bytecode);
        entry.locals = ((MethodBuilder &)(*(mit->second))).local_count();
        vtable.add_entry(entry);
      }
    }

    // Sort the virtual table according to the hash ids.
    std::sort(vtable.begin(), vtable.end(), c_vtable_comp);
		cm_count = vtable.size();

    cppcode += "static meat::vtable_entry_t " +
      class_name +
      "CMethods[] = {\n";

    bool first = true;
    std::vector<struct _c_vtable_entry_s>::const_iterator it;
    for (it = vtable.begin(); it != vtable.end(); it++) {
      if (!first) cppcode += ",\n";
      else first = false;

      cppcode += "  {" + it->str_hash + ", " + it->str_class_hash +
				", " + it->flags + ", " + ::to_string(it->locals) + ", " +
        it->func_name + "}"; // XXX Need to deal with the comma.
    }
    cppcode += "\n";

    cppcode += "};\n\n";
  }

  if (cls_props.size() > 0) {
    for (pit = cls_props.begin(); pit != cls_props.end(); pit++) {
      cppcode += std::string("#undef ") + (*pit)->to_string() + "\n";
    }
    cppcode += "\n";
  }

  /* Bytecode generation.
   */
  cpp_bytecode = bytecode.size();
  if (!bytecode.empty()) {
    cppcode += "static meat::uint8_t " + class_name + "Bytecode[] = {\n  ";
    bool first = true;
    for (unsigned int c = 0; c < bytecode.size(); c++) {
      if (!first) {
        if (c % 13 == 0)
          cppcode += ",\n  ";
        else
          cppcode += ", ";
      } else first = false;

      cppcode += itohex(bytecode[c], 2);
    }
    cppcode += "\n};\n\n";
  }

  return cppcode;
}

/*******************************
 * ClassBuilder::cpp_new_class *
 *******************************/

std::string meat::compiler::ClassBuilder::cpp_new_class() const {
  std::string cppcode;
  std::string class_name = this->property(0)->to_string();
  std::string cooked_name = cook_c_name(class_name.c_str());
  std::ostringstream hex;

  hex << std::hex << std::showbase << std::setw(8)
      << std::setfill('0');
  hex << ((Class &)(*super)).get_hash_id();

  cppcode += "\n  cls = new meat::Class(meat::Class::resolve(" + hex.str() +
    "), " + ::to_string(((List &)*(this->property(2))).size()) + ", " +
    ::to_string(((List &)*(this->property(1))).size() +
                CONST_CLASS(super).get_obj_properties()) + ");\n";

  if (!(this->property(5) == meat::Null())) {
      cppcode += "  cls->set_constructor(" + cooked_name + "_constructor);\n";
  }

  if (method_count() > 0)
    cppcode += "  cls->set_vtable(" + ::to_string(method_count()) +
      ", " + cooked_name + "Methods, meat::STATIC);\n";

  if (class_method_count() > 0)
    cppcode += "  cls->set_class_vtable(" + ::to_string(class_method_count()) +
      ", " + cooked_name + "CMethods, meat::STATIC);\n";

  if (cpp_bytecode > 0)
    cppcode += "  cls->set_bytecode(" + ::to_string(cpp_bytecode) +
      ", " + cooked_name + "Bytecode, meat::STATIC);\n";

  cppcode += std::string("  library.add(cls, \"") + class_name + "\");\n";

  return cppcode;
}

std::string meat::compiler::ClassBuilder::cpp_hash_id() const {
	uint32_t hash_id = hash(this->property(0)->to_string());
	std::stringstream hex;
  hex << std::setw(8) << std::setfill('0') << std::setbase(16);
  hex << hash_id;
  return ("0x" + hex.str());
}

/*************************
 * ClassBuilder::command *
 *************************/

void meat::compiler::ClassBuilder::command(Tokenizer &tokens) {

  if (tokens[0].type() == Token::WORD) {

    /* Class definitions */
    if (tokens[0] == "class") {

      if (tokens[1].type() == Token::WORD) {

        if (tokens[1] == "property") {
          cls_property(tokens[2]);

        } else if (tokens[1] == "method" || tokens[1] == "function") {

          uint8_t body_index = tokens.count() - 1;
          std::string method_name;

          // Build the method name
          for (int c = 2; c < body_index; c += 2) {
            method_name += (std::string &)(tokens[c]);
          }

#ifdef DEBUG
          std::cout << "COMPILE: class " << tokens[1] << " " << method_name
                    << std::endl;
#endif /* DEBUG */

          MethodBuilder *mb = new MethodBuilder(*this,
                                                (tokens[1] == "function"));

          Reference name = new Object(method_name.c_str());
          ((Index&)*(this->property(4)))[name] = mb;
          mb->property(0) = name;

          for (int c = 3; c < body_index; c += 2) {
            mb->add_parameter(tokens[c]);
          }

          mb->add_body(tokens[tokens.count() - 1]);
          //if (tokens[1] == "method")
          mb->compile();

        } else {
          throw Exception(
            "Expected class property, method or function keyword got class " +
            (std::string &)tokens[1]);
        }
      } else {
        throw Exception("Expected property or method keyword.");
      }

    } else {

      /* Object definitions. */
      if (tokens[0].type() == Token::WORD) {
        if (tokens[0] == "property") {
          obj_property(tokens[1]);

        } else if (tokens[0] == "constructor") {
          this->property(5) = new Object(((std::string &)tokens[1]).c_str());

        } else if (tokens[0] == "method" || tokens[0] == "function") {

          uint8_t body_index = tokens.count() - 1;
          std::string method_name;

          // Build the method name
          for (int c = 1; c < body_index; c += 2) {
            method_name += (std::string &)(tokens[c]);
          }

#ifdef DEBUG
          std::cout << "COMPILE: " << tokens[0] << " " << method_name << " as "
                    << std::hex << (unsigned int)hash(method_name.c_str())
                    << std::endl;
#endif /* DEBUG */

          // Create a new method builder and add it to the vtable mappings.
          MethodBuilder *mb = new MethodBuilder(*this,
                                                (tokens[0] == "function"));
          Reference name = new Object(method_name.c_str());
          ((Index&)*(this->property(3)))[name] = mb;
          mb->property(0) = name;

          // Add the method parameters to the method build..
          for (int c = 2; c < body_index; c += 2) {
            mb->add_parameter(tokens[c]);
          }

          // Add the body to the method builder and compile the method.
          mb->add_body(tokens[tokens.count() - 1]);
          //if (tokens[0] == "method")
          mb->compile();

        } else {
          throw Exception(
            "Expected property, method, function or constructor keyword got " +
            (std::string &)tokens[0]);
        }
      } else {
        throw Exception("Expected property or method keyword.");
      }

    }
  } else {
    throw Exception("Expected class, property, method or function keyword.");
  }

  tokens.clear();
}

/******************************
 * ClassBuilder::method_count *
 ******************************/

uint8_t meat::compiler::ClassBuilder::method_count() const {
	if (m_count == 0) {
		Index &methods = (Index &)(*(this->property(3)));
		return methods.size();
	} else
		return m_count;
}

/************************************
 * ClassBuilder::class_method_count *
 ************************************/

uint8_t meat::compiler::ClassBuilder::class_method_count() const {
	if (cm_count == 0) {
		Index &methods = (Index &)(*(this->property(4)));
		return methods.size();
	} else
		return cm_count;
}

/******************************************************************************
 * MethodBuilder Class Implementation
 */

/********************************
 * MethodBuilder::MethodBuilder *
 ********************************/

meat::compiler::MethodBuilder::MethodBuilder(ClassBuilder &cb, bool is_cpp)
  : Object(Class::resolve("Compiler.Method"), 3), cb(&cb), is_cpp(is_cpp) {
  //this->property(0) = meat::Null(); // Method name
  this->property(1) = new List();    // Parameters
  //this->property(2) = meat::Null(); // Code
};

/**************************
 * MethodBuilder::compile *
 **************************/

void meat::compiler::MethodBuilder::compile() {
  if (!is_cpp) {
    Reference super = cb->get_super();
    List &properties = (List &)(*(cb->property(1)));
    List &cls_properties = (List &)(*(cb->property(2)));

    meat::compiler::Method method(properties,
                                   CLASS(super).get_obj_properties(),
                                   cls_properties, 0);
    astree = (void *)&method;

    // Add the parameters to the AST Method node.
    List &parameters = (List &)(*(this->property(1)));
    meat::List::iterator it;
    for (it = parameters.begin(); it != parameters.end(); it++)
      method.add_parameter((*it)->to_string());

    // Tokenize and send command to command() method.
    execute(this->property(2)->to_string());

    method.gen_bytecode(true);  // Bytecode generation stage 1
    method.gen_bytecode(false); // Bytecode generation stage 2

    locals = method.locals();
    method.append_bytecode(bytecode);

    astree = NULL;
  }
}

/******************************
 * MethodBuilder::cpp_hash_id *
 ******************************/

std::string meat::compiler::MethodBuilder::cpp_hash_id() {
  uint32_t hash_id = hash(this->property(0)->to_string());
  std::stringstream hex;
  hex << std::setw(8) << std::setfill('0') << std::setbase(16);
  hex << hash_id;
  return ("0x" + hex.str());
}

/***************************
 * MethodBuilder::cpp_name *
 ***************************/

std::string meat::compiler::MethodBuilder::cpp_name(const char *prelim) {
  std::string orig_name = this->property(0)->to_string();

  // Transform operator names
  if (orig_name == "==") orig_name = "equals";
  if (orig_name == "!=") orig_name = "nequals";
  if (orig_name == "+") orig_name = "add";
  if (orig_name == "-") orig_name = "sub";
  if (orig_name == "*") orig_name = "mult";
  if (orig_name == "/") orig_name = "div";
  if (orig_name == "%") orig_name = "mod";
  if (orig_name == "^") orig_name = "pow";
  if (orig_name == "<") orig_name = "less";
  if (orig_name == "<=") orig_name = "less_equal";
  if (orig_name == ">") orig_name = "greater";
  if (orig_name == ">=") orig_name = "greater_equal";

  // Convert all : to _
  std::string method_name = std::string(prelim) + orig_name;
  for (unsigned int c = 0; c < method_name.size(); c++) {
    if (method_name[c] == ':')
      method_name[c] = '_';
  }
  return method_name;
}

/*****************************
 * MethodBuilder::cpp_method *
 *****************************/

std::string meat::compiler::MethodBuilder::cpp_method(const char *prelim) {
  if (is_cpp) {
    std::string code;

    code += "static meat::Reference " + cpp_name(prelim) +
      "(meat::Reference context) {\n";

    code += "  meat::Reference self = CONTEXT(context).get_self();\n";
    code += "  meat::Reference klass = CONTEXT(context).get_class();\n";

    List::const_iterator it;
    int i;
    for (i = 0, it = ((List &)*(this->property(1))).begin();
         it != ((List &)*(this->property(1))).end();
         it++, i++) {
      std::ostringstream convert;
      convert << i;
      code += std::string("  meat::Reference ") + (*it)->to_string() +
        " = CONTEXT(context).get_param(" + convert.str() + ");\n";
    }

    code += this->property(2)->to_string();
    code += "\n}\n\n";

    locals = ((List &)*(this->property(1))).size();

    return code;
  }

  return "";
}

/**************************
 * MethodBuilder::command *
 **************************/

void meat::compiler::MethodBuilder::command(Tokenizer &tokens) {
  meat::compiler::ASTNode &current = *((meat::compiler::ASTNode *)astree);

  if (tokens[1] == "=") {

#ifdef DEBUG
    std::cout << "COMPILER: Assignment " << tokens.to_string() << std::endl;
#endif

    meat::compiler::Value *dest =
      new meat::compiler::Value(tokens[0]);
    meat::compiler::ASTNode *src = NULL;

    switch (tokens[2].type()) {
    case Token::WORD:
      src = new meat::compiler::Value(tokens[2]);
      break;

    case Token::LITRL_STRING:
    case Token::SUBST_STRING:
      src = new meat::compiler::Value(tokens[2], true);
      break;

    case Token::COMMAND: {
      Tokenizer command_tokens;
      command_tokens.parse(tokens[2]);

      src = (meat::compiler::ASTNode *)parse_message(command_tokens);
      break;
    }
    case Token::BLOCK:
      throw Exception("Unable to assign blocks to a variable.");
		case Token::TEOF:
			throw Exception("Unexpected end of file in assignment.");
    }

    meat::compiler::Assignment *assign =
      new meat::compiler::Assignment(dest, src);
    current.add(assign);

  } else {
    meat::compiler::Message *mesg =
      (meat::compiler::Message *)parse_message(tokens);

    current.add(mesg);
  }

  tokens.clear();
}

/********************************
 * MethodBuilder::add_parameter *
 ********************************/

void compiler::MethodBuilder::add_parameter(const std::string &name) {
  List &parameters = dynamic_cast<List &>(*(this->property(1)));
  parameters.push_back(new Object(name.c_str()));
}

/***************************
 * MethodBuilder::add_body *
 ***************************/

void compiler::MethodBuilder::add_body(const std::string &body) {
  this->property(2) = new Object(body.c_str());
}

/*******************************
 * MethodBuilder::gen_bytecode *
 *******************************/

void compiler::MethodBuilder::gen_bytecode(std::vector<uint8_t> &class_bc) {

#ifdef DEBUG
  std::cout << "METHODBUILDER: Bytecode size = "
            << std::dec << this->bytecode.size()
            << std::endl;
#endif /* DEBUG */

  for (uint16_t c = 0; c < this->bytecode.size(); c++) {
#ifdef DEBUG
    /*std::cout << "  adding(" << std::hex << (unsigned int)class_bc.size()
      << ") " << (unsigned int)this->bytecode.at(c) << std::endl;*/
#endif /* DEBUG */
    class_bc.push_back(this->bytecode.at(c));
  }
}

/********************************
 * MethodBuilder::parse_message *
 ********************************/

void *compiler::MethodBuilder::parse_message(Tokenizer &tokens) {
  meat::compiler::ASTNode *obj = NULL;
  std::string method_name;
  bool super = false;

#ifdef DEBUG
  std::cout << "COMPILER: Message " << tokens.to_string() << std::endl;
#endif

  /* Determine the object that we are messaging. */
  switch (tokens[0].type()) {
  case Token::WORD:
    /* TODO
     * Keywords
     * - super
     */
    if (tokens[0] == "super") {
      super = true;
      obj = new meat::compiler::Value("self");
    } else
      obj = new meat::compiler::Value(tokens[0]);
    break;
  case Token::LITRL_STRING: // Messaging a constant string object.
  case Token::SUBST_STRING:
    obj = new meat::compiler::Value(tokens[0], true);
    break;
  case Token::COMMAND: {
    Tokenizer command_tokens;
    command_tokens.parse(tokens[0]);

    obj = (meat::compiler::ASTNode *)parse_message(command_tokens);
    break;
  }
  case Token::BLOCK:
    throw Exception("Messages cannot be sent directly to a block");
	case Token::TEOF:
			throw Exception("Unexpected end of file in message.");
  }

  /*  Grab every second word from the message to build the method name
   * from.
   */
  for (unsigned int c = 1; c < tokens.count(); c += 2) {
    method_name += (std::string &)(tokens[c]);
  }

  // Create the new message ast branch.
  meat::compiler::Message *mesg =
    new meat::compiler::Message(obj, method_name);
  mesg->message_super(super);

  /* Now add the parameters to the message ast branch.
   */
  if (tokens.count() > 2) {
    for (unsigned int c = 2; c < tokens.count(); c += 2) {
      switch (tokens[c].type()) {
      case Token::WORD:
        mesg->add_param(new meat::compiler::Value(tokens[c]));
        break;
      case Token::SUBST_STRING:
      case Token::LITRL_STRING:
        mesg->add_param(new meat::compiler::Value(tokens[c], true));
        break;
      case Token::BLOCK: {
        meat::compiler::Block *new_block = new meat::compiler::Block();
        void *save = astree;
        new_block->set_parent_block((meat::compiler::Block *)save);

        mesg->add_param(new_block);

        astree = (void *)new_block;
        Language::execute(tokens[c]);
        astree = save;

        break;
      }
      case Token::COMMAND: {
        Tokenizer command_tokens;
        command_tokens.parse(tokens[c]);

        meat::compiler::Message *mesg_param =
          (compiler::Message *)parse_message(command_tokens);
        mesg->add_param(mesg_param);

        break;
      }
			case Token::TEOF:
				throw Exception("Unexpected end of file.");
      }
    }
  }

  return mesg;
}
