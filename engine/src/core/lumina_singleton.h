#pragma once

// A class defined with this macro will be used as a singleton, once instanced there will be only 1 possible instance of it.
// Usage: Define a class passing the class name as follow (es: class LUMINA_SINGLETON_CLASS(class_name) {};)
// go to the cpp file of the class and assign the reference to the holding instance variable of the singleton
// (es: lumina::my_singleton_class_name* lumina::my_singleton_class_name::singleton_instance_ = nullptr;
// then instance the singleton somewhere in the application. You can the access it doing my_class::get_singleton() 
#define LUMINA_SINGLETON_CLASS(class_type) class_type : public lumina_singleton_t<class_type>

// Defines the singleton instance holding var declaration and assign's it with nullptr, it NEEDS to be called for every singleton class decleared once
#define LUMINA_SINGLETON_DECL_INSTANCE(class_type_with_namespace) class_type_with_namespace* class_type_with_namespace::singleton_instance_ = nullptr

// Creates the singleton instance
#define LUMINA_SINGLETON_INIT_INSTANCE(class_type) class_type* class_type##_instance = new class_type();

// Destroys the singleton instance
#define LUMINA_SINGLETON_DESTROY_INSTANCE(class_type) delete& class_type::get_singleton();

template<class singleton_class_type>
struct lumina_singleton_t
{
public:

	lumina_singleton_t() { singleton_instance_ = reinterpret_cast<singleton_class_type*>(this); }

	static singleton_class_type& get_singleton() { return *singleton_instance_; }

private:

	static singleton_class_type* singleton_instance_;

};