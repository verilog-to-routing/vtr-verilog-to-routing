// file      : tests/parser/driver.cxx
// license   : MIT; see accompanying LICENSE file

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include <libstudxml/parser.hxx>

#undef NDEBUG
#include <cassert>

using namespace std;
using namespace xml;

int
main ()
{
  // Test error handling.
  //
  try
  {
    istringstream is ("<root><nested>X</nasted></root>");
    parser p (is, "test");

    assert (p.next () == parser::start_element);
    assert (p.next () == parser::start_element);
    assert (p.next () == parser::characters && p.value () == "X");
    p.next ();
    assert (false);
  }
  catch (const xml::exception&)
  {
    // cerr << e.what () << endl;
  }

  try
  {
    istringstream is ("<root/>");
    parser p (is, "test");

    is.setstate (ios_base::badbit);
    p.next ();
    assert (false);
  }
  catch (const xml::exception&)
  {
  }

  // Test the next_expect() functionality.
  //
  {
    istringstream is ("<root/>");
    parser p (is, "test");
    p.next_expect (parser::start_element, "root");
    p.next_expect (parser::end_element);
  }

  try
  {
    istringstream is ("<root/>");
    parser p (is, "test");
    p.next_expect (parser::end_element);
    assert (false);
  }
  catch (const xml::exception&)
  {
    // cerr << e.what () << endl;
  }

  try
  {
    istringstream is ("<root/>");
    parser p (is, "test");
    p.next_expect (parser::start_element, "root1");
    assert (false);
  }
  catch (const xml::exception&)
  {
    // cerr << e.what () << endl;
  }

  // Test next_expect() with content setting.
  //
  {
    istringstream is ("<root>  </root>");
    parser p (is, "empty");

    p.next_expect (parser::start_element, "root", content::empty);
    p.next_expect (parser::end_element);
    p.next_expect (parser::eof);
  }

  // Test namespace declarations.
  //
  {
    // Followup end element event that should be precedeeded by end
    // namespace declaration.
    //
    istringstream is ("<root xmlns:a='a'/>");
    parser p (is,
              "test",
              parser::receive_default |
              parser::receive_namespace_decls);

    p.next_expect (parser::start_element, "root");
    p.next_expect (parser::start_namespace_decl);
    p.next_expect (parser::end_namespace_decl);
    p.next_expect (parser::end_element);
  }

  // Test value extraction.
  //
  {
    istringstream is ("<root>123</root>");
    parser p (is, "test");
    p.next_expect (parser::start_element, "root");
    p.next_expect (parser::characters);
    assert (p.value<int> () == 123);
    p.next_expect (parser::end_element);
  }

  // Test attribute maps.
  //
  {
    istringstream is ("<root a='a' b='b' d='123' t='true'/>");
    parser p (is, "test");
    p.next_expect (parser::start_element, "root");

    assert (p.attribute ("a") == "a");
    assert (p.attribute ("b", "B") == "b");
    assert (p.attribute ("c", "C") == "C");
    assert (p.attribute<int> ("d") == 123);
    assert (p.attribute<bool> ("t") == true);
    assert (p.attribute ("f", false) == false);

    p.next_expect (parser::end_element);
  }

  {
    istringstream is ("<root a='a'><nested a='A'><inner/></nested></root>");
    parser p (is, "test");
    p.next_expect (parser::start_element, "root");
    assert (p.attribute ("a") == "a");
    assert (p.peek () == parser::start_element && p.name () == "nested");
    assert (p.attribute ("a") == "a");
    p.next_expect (parser::start_element, "nested");
    assert (p.attribute ("a") == "A");
    p.next_expect (parser::start_element, "inner");
    assert (p.attribute ("a", "") == "");
    p.next_expect (parser::end_element);
    assert (p.attribute ("a") == "A");
    assert (p.peek () == parser::end_element);
    assert (p.attribute ("a") == "A"); // Still valid.
    p.next_expect (parser::end_element);
    assert (p.attribute ("a") == "a");
    p.next_expect (parser::end_element);
    assert (p.attribute ("a", "") == "");
  }

  try
  {
    istringstream is ("<root a='a' b='b'/>");
    parser p (is, "test");
    p.next_expect (parser::start_element, "root");
    assert (p.attribute ("a") == "a");
    p.next_expect (parser::end_element);
    assert (false);
  }
  catch (const xml::exception&)
  {
    // cerr << e.what () << endl;
  }

  try
  {
    istringstream is ("<root a='abc'/>");
    parser p (is, "test");
    p.next_expect (parser::start_element, "root");
    p.attribute<int> ("a");
    assert (false);
  }
  catch (const xml::exception&)
  {
    // cerr << e.what () << endl;
  }

  // Test peeking and getting the current event.
  //
  {
    istringstream is ("<root x='x'>x<nested/></root>");
    parser p (is, "peek",
              parser::receive_default | parser::receive_attributes_event);

    assert (p.event () == parser::eof);

    assert (p.peek () == parser::start_element);
    assert (p.next () == parser::start_element);
    assert (p.event () == parser::start_element);

    assert (p.peek () == parser::start_attribute);
    assert (p.event () == parser::start_attribute);
    assert (p.next () == parser::start_attribute);

    assert (p.peek () == parser::characters && p.value () == "x");
    assert (p.next () == parser::characters && p.value () == "x");
    assert (p.event () == parser::characters && p.value () == "x");

    assert (p.peek () == parser::end_attribute);
    assert (p.event () == parser::end_attribute);
    assert (p.next () == parser::end_attribute);

    assert (p.peek () == parser::characters && p.value () == "x");
    assert (p.next () == parser::characters && p.value () == "x");
    assert (p.event () == parser::characters && p.value () == "x");

    assert (p.peek () == parser::start_element);
    assert (p.next () == parser::start_element);
    assert (p.event () == parser::start_element);

    assert (p.peek () == parser::end_element);
    assert (p.next () == parser::end_element);
    assert (p.event () == parser::end_element);

    assert (p.peek () == parser::end_element);
    assert (p.next () == parser::end_element);
    assert (p.event () == parser::end_element);

    assert (p.peek () == parser::eof);
    assert (p.next () == parser::eof);
    assert (p.event () == parser::eof);
  }

  // Test content processing.
  //

  // empty
  //
  {
    istringstream is ("<root x=' x '>  \n\t </root>");
    parser p (is, "empty",
              parser::receive_default | parser::receive_attributes_event);

    assert (p.next () == parser::start_element);
    p.content (content::empty);
    assert (p.next () == parser::start_attribute);
    assert (p.next () == parser::characters && p.value () == " x ");
    assert (p.next () == parser::end_attribute);
    assert (p.next () == parser::end_element);
    assert (p.next () == parser::eof);
  }

  try
  {
    istringstream is ("<root>  \n &amp; X \t </root>");
    parser p (is, "empty");

    assert (p.next () == parser::start_element);
    p.content (content::empty);
    p.next ();
    assert (false);
  }
  catch (const xml::exception&)
  {
    // cerr << e.what () << endl;
  }

  // simple
  //
  {
    istringstream is ("<root> X </root>");
    parser p (is, "simple");

    assert (p.next () == parser::start_element);
    p.content (content::simple);
    assert (p.next () == parser::characters && p.value () == " X ");
    assert (p.next () == parser::end_element);
    assert (p.next () == parser::eof);
  }

  try
  {
    istringstream is ("<root> ? <nested/></root>");
    parser p (is, "simple");

    assert (p.next () == parser::start_element);
    p.content (content::simple);
    assert (p.next () == parser::characters && p.value () == " ? ");
    p.next ();
    assert (false);
  }
  catch (const xml::exception&)
  {
    // cerr << e.what () << endl;
  }

  {
    // Test content accumulation in simple content.
    //
    istringstream is ("<root xmlns:a='a'>1&#x32;3</root>");
    parser p (is,
              "simple",
              parser::receive_default |
              parser::receive_namespace_decls);

    assert (p.next () == parser::start_element);
    p.next_expect (parser::start_namespace_decl);
    p.content (content::simple);
    assert (p.next () == parser::characters && p.value () == "123");
    p.next_expect (parser::end_namespace_decl);
    assert (p.next () == parser::end_element);
    assert (p.next () == parser::eof);
  }

  try
  {
    // Test error handling in accumulation in simple content.
    //
    istringstream is ("<root xmlns:a='a'>1&#x32;<nested/>3</root>");
    parser p (is,
              "simple",
              parser::receive_default |
              parser::receive_namespace_decls);

    assert (p.next () == parser::start_element);
    p.next_expect (parser::start_namespace_decl);
    p.content (content::simple);
    p.next ();
    assert (false);
  }
  catch (const xml::exception&)
  {
    // cerr << e.what () << endl;
  }

  // complex
  //
  {
    istringstream is ("<root x=' x '>\n"
                      "  <nested>\n"
                      "    <inner/>\n"
                      "    <inner> X </inner>\n"
                      "  </nested>\n"
                      "</root>\n");
    parser p (is, "complex",
              parser::receive_default | parser::receive_attributes_event);

    assert (p.next () == parser::start_element); // root
    p.content (content::complex);

    assert (p.next () == parser::start_attribute);
    assert (p.next () == parser::characters && p.value () == " x ");
    assert (p.next () == parser::end_attribute);

    assert (p.next () == parser::start_element); // nested
    p.content (content::complex);

    assert (p.next () == parser::start_element); // inner
    p.content (content::empty);
    assert (p.next () == parser::end_element);   // inner

    assert (p.next () == parser::start_element); // inner
    p.content (content::simple);
    assert (p.next () == parser::characters && p.value () == " X ");
    assert (p.next () == parser::end_element);   // inner

    assert (p.next () == parser::end_element);   // nested
    assert (p.next () == parser::end_element);   // root
    assert (p.next () == parser::eof);
  }

  try
  {
    istringstream is ("<root> \n<n/> X <n> X </n>  </root>");
    parser p (is, "complex");

    assert (p.next () == parser::start_element);
    p.content (content::complex);
    assert (p.next () == parser::start_element);
    assert (p.next () == parser::end_element);
    p.next ();
    assert (false);
  }
  catch (const xml::exception&)
  {
    // cerr << e.what () << endl;
  }

  // Test element with simple content helpers.
  //
  {
    istringstream is ("<root>"
                      "  <nested>X</nested>"
                      "  <nested/>"
                      "  <nested>123</nested>"
                      "  <nested>Y</nested>"
                      "  <t:nested xmlns:t='test'>Z</t:nested>"
                      "  <nested>234</nested>"
                      "  <t:nested xmlns:t='test'>345</t:nested>"
                      "  <nested>A</nested>"
                      "  <t:nested xmlns:t='test'>B</t:nested>"
                      "  <nested1>A</nested1>"
                      "  <t:nested1 xmlns:t='test'>B</t:nested1>"
                      "  <nested>1</nested>"
                      "  <t:nested xmlns:t='test'>2</t:nested>"
                      "  <nested1>1</nested1>"
                      "  <t:nested1 xmlns:t='test'>2</t:nested1>"
                      "</root>");
    parser p (is, "element");

    p.next_expect (parser::start_element, "root", content::complex);

    p.next_expect (parser::start_element, "nested");
    assert (p.element () == "X");

    p.next_expect (parser::start_element, "nested");
    assert (p.element () == "");

    p.next_expect (parser::start_element, "nested");
    assert (p.element<unsigned int> () == 123);

    assert (p.element ("nested") == "Y");
    assert (p.element (qname ("test", "nested")) == "Z");

    assert (p.element<unsigned int> ("nested") == 234);
    assert (p.element<unsigned int> (qname ("test", "nested")) == 345);

    assert (p.element ("nested", "a") == "A");
    assert (p.element (qname ("test", "nested"), "b") == "B");

    assert (p.element ("nested", "a") == "a" &&
            p.element ("nested1") == "A");
    assert (p.element (qname ("test", "nested"), "b") == "b" &&
            p.element (qname ("test", "nested1")) == "B");

    assert (p.element<unsigned int> ("nested", 10) == 1);
    assert (p.element<unsigned int> (qname ("test", "nested"), 20) == 2);

    assert (p.element<unsigned int> ("nested", 10) == 10 &&
            p.element<unsigned int> ("nested1") == 1);
    assert (p.element<unsigned int> (qname ("test", "nested"), 20) == 20 &&
            p.element<unsigned int> (qname ("test", "nested1")) == 2);

    p.next_expect (parser::end_element);
  }

  // Test the iterator interface.
  //
  {
    istringstream is ("<root><nested>X</nested></root>");
    parser p (is, "iterator");

    vector<parser::event_type> v;

    for (parser::iterator i (p.begin ()); i != p.end (); ++i)
      v.push_back (*i);

    //for (parser::event_type e: p)
    //  v.push_back (e);

    assert (v.size () == 5);
    assert (v[0] == parser::start_element);
    assert (v[1] == parser::start_element);
    assert (v[2] == parser::characters);
    assert (v[3] == parser::end_element);
    assert (v[4] == parser::end_element);
  }

  // Test space extraction into the std::string value.
  //
  {
    istringstream is ("<root a=' a '> b </root>");
    parser p (is, "test");
    p.next_expect (parser::start_element, "root");
    assert (p.attribute<std::string> ("a") == " a ");
    p.next_expect (parser::characters);
    assert (p.value<std::string> () == " b ");
    p.next_expect (parser::end_element);
  }
}
