// file      : examples/hybrid/dom.hxx
// copyright : not copyrighted - public domain

#ifndef DOM_HXX
#define DOM_HXX

#include <map>
#include <string>
#include <vector>

#include <libstudxml/qname.hxx>
#include <libstudxml/forward.hxx>

// A simple, DOM-like in-memory representation of raw XML. It only supports
// empty, simple, and complex content (no mixed content) and is not
// particularly efficient, at least not in C++98.

class element;
typedef std::map<xml::qname, std::string> attributes;
typedef std::vector<element> elements;

class element
{
public:
  typedef ::attributes attributes_type;
  typedef ::elements elements_type;

  element (const xml::qname& name): name_ (name) {}
  element (const xml::qname& name, const std::string text)
      : name_ (name), text_ (text) {}

  const xml::qname&
  name () const {return name_;}

  const attributes_type&
  attributes () const {return attributes_;}

  attributes_type&
  attributes () {return attributes_;}

  const std::string&
  text () const {return text_;}

  void
  text (const std::string& text) {text_ = text;}

  const elements_type&
  elements () const {return elements_;}

  elements_type&
  elements () {return elements_;}

public:
  // Parse an element. If start_end is false, then don't parse the
  // start and end of the element.
  //
  element (xml::parser&, bool start_end = true);

  // Serialize an element. If start_end is false, then don't serialize
  // the start and end of the element.
  //
  void
  serialize (xml::serializer&, bool start_end = true) const;

private:
  xml::qname name_;
  attributes_type attributes_;
  std::string text_;           // Simple content only.
  elements_type elements_;     // Complex content only.
};

#endif // DOM_HXX
