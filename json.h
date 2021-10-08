//
// Created by Pluto on 2021/10/2.
//
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cassert>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdexcept>

typedef std::string String;

namespace json {
    enum json_value_type {
        JSON_NULL,     //空类型
        JSON_BOOL,     //bool类型
        JSON_NUMBER,   //int，double类型
        JSON_STRING,   //字符串类型
        JSON_ARRAY,      //字典类型
        JSON_OBJECT      //字典类型
    };

    class json {
    public:
        json() : type(JSON_NULL) {}

        virtual ~json() = default;

        json_value_type get_type() const { return type; }

        virtual void write(std::ostream &os, int indent = 0) {}

        virtual String &get_string() { throw std::runtime_error("It's not a string"); }

        virtual double &get_double() { throw std::runtime_error("It's not a number"); }

        virtual bool &get_bool() { throw std::runtime_error("It's not a bool"); }

        virtual std::unordered_map<String, std::shared_ptr<json>> &get_object() {
            throw std::runtime_error("It's not an object");
        }

        virtual std::vector<std::shared_ptr<json>> &get_array() {
            throw std::runtime_error("It's not an array");
        }

        virtual std::shared_ptr<json> &operator[](size_t index) {
            throw std::runtime_error("Not array");
        }

        virtual std::shared_ptr<json> &operator[](const String &s) {
            throw std::runtime_error("Not object");
        }

        void write_in_file(const String &filename) {
            std::ofstream out(filename);
            if (!out) throw (std::runtime_error("Can not open" + filename));
            write(out, 0);
        }

    protected:
        static void process_string(std::ostream &out, const std::string &origin) {
            std::ostringstream oss;
            oss << '"';
            for (auto i: origin) {
                switch (i) {
                    case '"': {
                        oss << '\\' << '"';
                        break;
                    }
                    case '\n': {
                        oss << '\\' << 'n';
                        break;
                    }
                    case '\\': {
                        oss << '\\' << '\\';
                        break;
                    }
                    default:
                        oss << i;
                        break;
                }
            }
            oss << '"';
            out << oss.str();
        }

        static void indent(std::ostream &os, int depth) {
            for (int i = 0; i < depth; ++i)
                os << '\t';
        }

    protected:
        json_value_type type;
    };

    class jsonString : public json {
    private:
        String str;
    public:
        jsonString() { type = JSON_STRING; }

        jsonString(String s) : str(std::move(s)) { type = JSON_STRING; }

//        json_value_type get_type() const final { return type; }

        void write(std::ostream &out, int indent = 0) final { process_string(out, str); }

        String &get_string() final { return str; }
    };

    class jsonNumber : public json {
    private:
        double value;
    public:
        jsonNumber() : value(0) { type = JSON_NUMBER; }

        jsonNumber(const double &num) : value(num) { type = JSON_NUMBER; }

//        json_value_type get_type() const final { return type; }

        void write(std::ostream &out, int ident = 0) final { out << value; }

        double &get_double() final { return value; }
    };

    class jsonBool : public json {
    private:
        bool boolean;
    public:
        jsonBool() : boolean(false) { type = JSON_BOOL; }

        jsonBool(const bool &b) : boolean(b) { type = JSON_BOOL; }

//        json_value_type get_type() const final { return type; }

        void write(std::ostream &out, int ident = 0) final { out << (boolean ? "true" : "false"); }

        bool &get_bool() final { return boolean; }
    };

    class jsonArray : public json {
    private:
        std::vector<std::shared_ptr<json>> ptr_vec;
    public:
        jsonArray() { type = JSON_ARRAY; }

        void write(std::ostream &out, int idt = 0) final {
            out << '[';
            if (ptr_vec.empty()) {
                out << ']';
                return;
            }
            bool is_first = true;
            for (auto &it: ptr_vec) {
                if (is_first)
                    is_first = false;
                else
                    out << ',';
                out << '\n';
                indent(out, idt);
                it->write(out, idt + 1);
            }
            out << '\n';
            indent(out, idt-1);
            out << ']';
        }

//        json_value_type get_type() const final {return type;}
        std::vector<std::shared_ptr<json>> &get_array() final { return ptr_vec; }

        std::shared_ptr<json> &operator[](size_t index) final { return ptr_vec[index]; }
    };

    class jsonObject : public json {
    private:
        std::unordered_map<String, std::shared_ptr<json>> obj;
    public:
        jsonObject() { type = JSON_OBJECT; }

        void write(std::ostream &out, int idt = 0) final;

        std::unordered_map<String, std::shared_ptr<json>> &get_object() final {
            return obj;
        }

        std::shared_ptr<json> &operator[](const String &key) final { return obj[key]; }

        template<class T>
        void insert(const String &key, T value) {
            obj[key] = std::make_shared<json>(value);
        }
    };

    inline
    void jsonObject::write(std::ostream &out, int idt) {
        out << '{';
        if (obj.empty()) {
            out << '}';
            return;
        }
        bool is_first = true;
        for (auto &item: obj) {
            if (is_first) is_first = false;
            else out << ',';
            out << '\n';
            indent(out, idt);
            process_string(out, item.first);
            out << ':';
            item.second->write(out, idt + 1);
        }
        out << '\n';
        indent(out, idt-1);
        out << '}';
    }

    inline
    std::shared_ptr<json> parse(std::istream &in) {
        //删掉空格
        auto eat_whitespace = [&in]() -> char {
            char letter;
            in.get(letter);
            while (letter == ' ' || letter == '\t' || letter == '\n' || letter == '\r')
                in.get(letter);
            return letter;
        };

        //处理字符串
        auto parse_string = [&in]() -> String {
            char letter;
            in.get(letter);
            std::stringstream ss;
            while (letter != '"') {
                if (letter == '\\') {
                    char p = static_cast<char>(in.get());
                    switch (p) {
                        case '"':
                            ss << '"';
                            break;
                        case 'n':
                            ss << '\n';
                            break;
                        case '\'':
                            ss << '\'';
                            break;
                        case 'b':
                            ss << '\b';
                            break;
                        case 'f':
                            ss << '\f';
                            break;
                        case 'r':
                            ss << '\r';
                            break;
                        case 't':
                            ss << '\t';
                            break;
                        default:
                            throw std::runtime_error("When parsing string INVALID char" + p);
                    }
                } else ss << letter;
                in.get(letter);
            }
            return ss.str();
        };

        auto parse_number = [&in]() -> double {
            std::stringstream ss;
            double ret;
            in.unget();
            char ch = static_cast<char>(in.get());
            while ((!in.fail()) && (ch == '-' || ch == 'e' || ch == 'E' || ch == '.' || ch == ',' || ch == '+' ||
                                    ch == '-' || (ch >= '0' && ch <= '9'))) {
//                ch=static_cast<char>(in.get());
                ss << ch;
                in.get(ch);
            }
            in.unget();
            ss>>ret;
            return ret;
        };


        auto parse_bool = [&in]() -> bool {
            in.unget();
            char boolean;
            in.get(boolean);
            if (boolean == 't') {
                if (in.get() == 'r' && in.get() == 'u' && in.get() == 'e')
                    return true;
                else
                    throw std::runtime_error("Invalid input when parsing bool 'true'");
            }
//            return true;
        };

        auto parse_null = [&in]() -> String {
            in.unget();
            char boolean;
            in.get(boolean);
            if (boolean == 'n') {
                if (in.get() == 'u' && in.get() == 'l' && in.get() == 'l')
                    return "null";
                else
                    throw std::runtime_error("Invalid input when parsing null");
            }
//            return true;
        };

        char letter = eat_whitespace();//第一个非空字符
        if (!in.good()) {
//            json js;
            return std::make_shared<json>();
        }
        if (letter == '"') return std::make_shared<jsonString>(parse_string());
        else if (letter == '-' || (letter >= '0' && letter <= '9'))
            return std::make_shared<jsonNumber>(parse_number());
        else if (letter == 't' || letter == 'f')
            return std::make_shared<jsonBool>(parse_bool());
        else if (letter == 'n') {
//            if (in.get() == 'u' && in.get() == 'l' && in.get() == 'l')
                return std::make_shared<jsonString>(parse_null());
//            else
//                throw std::runtime_error("Invalid input when parsing 'null'");
        } else if (letter == '[') {
            auto ret = std::make_shared<jsonArray>();
            do {
                letter = eat_whitespace();
                if (letter == ',') letter = eat_whitespace();
                in.unget();
                ret->get_array().push_back(parse(in));
//                if(letter!=']')
                letter = in.peek();
                if(letter == ' ' || letter == '\t' || letter == '\n' || letter == '\r')
                    letter=eat_whitespace();//in文件流中的下一字符
            } while (letter != ']');
            in.get();
            return std::move(ret);
        } else if (letter == '{') {
            auto ret = std::make_shared<jsonObject>();
            do {
                letter = eat_whitespace();
                if (letter == ',') letter = eat_whitespace();
                if (letter == '"') {
                    String key = parse_string();
                    letter = eat_whitespace();
                    if (letter != ':')
                        throw std::runtime_error("object error");
                    ret->get_object()[key] = parse(in);
                    letter=in.peek();
//                    if(letter == ' ' || letter == '\t' || letter == '\n' || letter == '\r')
//                        letter=eat_whitespace();
                } else break;
            } while (letter != '}');
            return std::move(ret);
        }
        else {
            throw std::runtime_error("unexpected character"+letter);
        }
    }
}


