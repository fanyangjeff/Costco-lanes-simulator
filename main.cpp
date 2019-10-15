//
//  main.cpp
//  hw3
//
//  Created by YF on 20/05/2019.
//  Copyright © 2019 YF. All rights reserved.
//
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <bitset>
#include <list>
#include <condition_variable> // std::condition_variable
#include <memory>
#include <mutex>
#include <thread>
#include <ctime>
#include <deque>

using namespace std;
const int PRIME = 999983;
const int NUM_LANES = 20;
std::mutex mtx;
std::condition_variable cv;
bool ready = false;
std::vector<string> CHARACTERS = {"-","$","%","*",".", "/", "+", "0", "1", "2", "3", "4", "5", "6", "7", "8",
    "9", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
    "O", "P", "Q", "R", "S", "^", "T", "U", "V", "W", "X", "Y", "Z"};

std::vector<string> SYMBOLS = {"nwnnnnwnw", "nwnwnwnnn", "nnnwnwnwn", "nwnnwnwnn", "wwnnnnwnn",
    "nwnwnnnwn", "nwnnnwnwn", "nnnwwnwnn", "wnnwnnnnw", "nnwwnnnnw", "wnwwnnnnn", "nnnwwnnnw", "wnnwwnnnn",
    "nnwwwnnnn", "nnnwnnwnw", "wnnwnnwnn", "nnwwnnwnn", "wnnnnwnnw", "nnwnnwnnw", "wnwnnwnnn", "nnnnwwnnw",
    "wnnnwwnnn", "nnwnwwnnn", "nnnnnwwnw", "wnnnnwwnn", "nnwnnwwnn", "nnnnwwwnn", "wnnnnnnww", "nnwnnnnww",
    "wnwnnnnwn", "nnnnwnnww", "wnnnwnnwn", "nnwnwnnwn", "nnnnnnwww", "wnnnnnwwn", "nnwnnnwwn", "nwwnnnwnn",
    "nnnnwnwwn", "wwnnnnnnw", "nwwnnnnnw", "wwwnnnnnn", "nwnnwnnnw", "wwnnwnnnn", "nwwnwnnnn"};

std::vector<char> HEXI = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
std::vector<string> HEXI_STR = {"0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", "1000",
    "1001", "1010", "1011", "1100", "1101", "1110", "1111"};



class Product_DataBase
{
private:
    std::mutex mtx;
    shared_ptr<vector<pair<std::string, std::string>>> item_table;
    shared_ptr<vector<std::string>> character_table;
    shared_ptr<vector<string>> hexi_table;
    
public:
    Product_DataBase()
    {
        item_table = shared_ptr<vector<pair<string, string>>> (new vector<pair<string, string>> ( PRIME + 1));
        character_table = shared_ptr<vector<string>> (new vector<string> (512, ""));
        hexi_table = shared_ptr<vector<string>> (new vector<string> (100, ""));
        
        for(auto i = 0; i < HEXI.size(); i++)
            hexi_table->at(int(HEXI[i])) = HEXI_STR[i];
        this->build_char_table();
        
    }
    
    void build_char_table()
    {
        for(auto i = 0; i < CHARACTERS.size(); i++)
        {
            regex pattern1("w");
            regex pattern2("n");
            std::string s = regex_replace(SYMBOLS[i], pattern1, "1");
            s = regex_replace(s, pattern2, "0");
            bitset<9> temp(s);
            
            character_table->at(temp.to_ulong()) = CHARACTERS[i];
            
        }
    }
    
    void biuld_item_table(int index, std::vector<std::string> item)
    {
        list<int> indices = this->find_item_abbrivation(item[1]);
        std::string name = "";
        for(auto pos: indices)
            name += character_table->at(pos);
        
        std::pair<std::string, std::string> temp_pair(name, item[2]);
        item_table->at(index) = temp_pair;
    }
    
    
    void get_item(string barcode, pair<string, string>& target)
    {
        std::unique_lock<std::mutex> lck(mtx);
        
        while(!ready)
        {
            cv.wait(lck);
        }
        auto func =[](string barcode) -> size_t
        {
            if (barcode == "")
                throw "Barcode missing";
            else
            {
                return std::hash<bitset<48>>{} (bitset<48> (std::string(barcode))) % PRIME;
            }
        };
        
        size_t index = func(barcode);
        target = item_table->at(index);

    }
    
    string translate_barcode(string s)
    {
        string code = "";
        for(auto i = 0; i < s.size(); i++)
        {
            code += get_partial_code(int(static_cast<char>(s[i])));
        }
        return code;
    }
    
    string get_partial_code(int index){return this->hexi_table->at(index);}
    
    
    list<int> find_item_abbrivation(std::string s)
    {
        list<int> indices;
        for(int i = 0; i < 5; i++)
        {
            bitset<9> temp(std::string(s.substr(9 * i, 9)));
            indices.push_back(int(temp.to_ulong()));
        }
        
        return indices;
    }
    
};


class FileHandler1
{
private:
    string filename;
    ifstream in;
public:
    FileHandler1(string filename){this->filename = filename;}
    void openFile(shared_ptr<Product_DataBase> hashtable)
    {
        in.open(filename);
        if(!in)
        {
            cout<<"File cannot be opened\n";
            exit(EXIT_FAILURE);
        }
        while(in)
        {
            std::vector<std::string> temp;
            for(int i = 0; i < 4; i++)
            {
                string s;
                getline(in, s);
                regex pattern("[a-zA-z/<>\\s]");
                string result = std::regex_replace(s, pattern, "");
                temp.push_back(result);
            }
            try
            {
                auto hashfunc = [](std::vector<std::string> v) -> size_t
                {
                    if (v[1] == "")
                        throw "Barcode missing";
                    else
                    {
                        return std::hash<bitset<48>>{} (bitset<48> (std::string(v[1]))) % PRIME;
                    }
                };
                size_t index = hashfunc(temp);        //hash each barcode
                //cout<<index<<endl;
                hashtable->biuld_item_table(int(index), temp);
            }
            
            catch (const char* msg) {
                //cerr << msg << endl;
                continue;
            }
            catch (std::invalid_argument) {
                cerr <<"bad" << endl;
            }
            
        }
    }
};

class Cart
{
private:
    shared_ptr<std::string> id;
    shared_ptr<double> total;
    int current_item_num;
    shared_ptr<list<string>> raw_barcodes;
    shared_ptr<vector<string>> translated_barcodes;
    shared_ptr<std::vector<shared_ptr<pair<string, string>>>> items;
    
public:
    Cart()
    {
        id = shared_ptr<std::string>(new std::string);
        total = shared_ptr<double>(new double(0));
        items = shared_ptr<std::vector<shared_ptr<pair<string, string>>>>(new vector<shared_ptr<pair<string, string>>> (100));
        raw_barcodes = shared_ptr<list<string>> (new list<string>);
        translated_barcodes = shared_ptr<vector<string>>(new vector<string>);
        current_item_num = 0;
    }
    
    Cart(std::string id)
    {
        this->id = shared_ptr<std::string>(new std::string(id));
        total = shared_ptr<double>(new double(0));
        items = shared_ptr<std::vector<shared_ptr<pair<string, string>>>>(new vector<shared_ptr<pair<string, string>>> ( 100 ));
        raw_barcodes = shared_ptr<list<string>> (new list<string>);
        translated_barcodes = shared_ptr<vector<string>>(new vector<string>);
        current_item_num = 0;
    }
    
    void process_barcodes(string target, shared_ptr<Product_DataBase> table)
    {
        sregex_token_iterator end;
        regex pattern("[A-F0-9]+");
        
        for (sregex_token_iterator pos(target.begin(), target.end(), pattern);pos != end; ++pos)
            raw_barcodes->push_back(*pos);
        
        //then we need to process each barcode(string) into binary barcode
        
        for(auto it = raw_barcodes->begin(); it != raw_barcodes->end(); it++)
        {
            translated_barcodes->push_back(table->translate_barcode(*it));
        }
    }
    
    shared_ptr<vector<string>> get_barcodes(){return translated_barcodes;}

    void add_new_item(pair<string, string> new_item)
    {
        items->at(current_item_num) = shared_ptr<pair<string, string>>(new pair<string, string>);
        (*items->at(current_item_num)) = new_item;
        (*total) += stod(new_item.second);
        current_item_num ++;
    }
    
    std::string get_name(){return *id;}
    
    void set_id(string id){(*this->id) = id;}
    
    void update_total(double price) {(*this->total) += price;}
    
    int get_num_items(){return current_item_num;}
    
    void helper()
    {
        for(auto it = translated_barcodes->begin();  it != translated_barcodes->end(); it ++)
            cout<<*(it)<<endl;
    }
    
    void print_receipt(int line_id)
    {
        cout<<"This is line "<<line_id<<". Done scanning all the items from "<<*id<<endl;
    
        
        for(auto i = 0; i < current_item_num; i++)
        {
            cout<<items->at(i)->first<<" "<<items->at(i)->second<<endl;
        }
        
        cout<<"Total: "<<*total<<endl<<endl<<endl;
        
    }
};



class Lane
{
private:
    shared_ptr<deque<shared_ptr<Cart>>> lane;
public:
    Lane()
    {
        lane = shared_ptr<deque<shared_ptr<Cart>>>(new deque<shared_ptr<Cart>>);
    }
    void push_a_cart(shared_ptr<Cart> newItem)
    {
        lane->push_back(newItem);
    }
    
    deque<shared_ptr<Cart>> get_carts(){return *lane;}
    void helper()
    {
        while(lane->size())
        {
            shared_ptr<Cart> temp = lane->front();
            lane->pop_front();
            cout<<temp->get_name()<<endl;
            shared_ptr<vector<string>> barcodes = temp->get_barcodes();
            cout<<barcodes->size()<<endl;
            for(auto it = barcodes->begin(); it != barcodes->end(); it ++)
                cout<<(*it)<<endl;
        }
    }
};



void process_lanes(shared_ptr<Lane> queue, shared_ptr<Product_DataBase> table, int line_id)
{
    deque<shared_ptr<Cart>> carts = queue->get_carts();
    while(carts.size())
    {
        shared_ptr<Cart> temp_cart = carts.front();
        carts.pop_front();
        shared_ptr<vector<string>> barcodes =temp_cart->get_barcodes();
        
        for(auto i = 0; i < barcodes->size(); i++)
        {
            pair<string, string> item;
            table->get_item(barcodes->at(i), item);
            mtx.lock();
            temp_cart->add_new_item(item);
            mtx.unlock();
        }
        
        mtx.lock();
        temp_cart->print_receipt(line_id);
        mtx.unlock();
    }
    
    return;
}
     



class CheckOutLanes
{
private:
    shared_ptr<vector<shared_ptr<Lane>>> lanes;
    shared_ptr<vector<shared_ptr<thread>>> threads;
    int num_of_carts;
public:
    CheckOutLanes()
    {
        lanes = shared_ptr<vector<shared_ptr<Lane>>>(new vector<shared_ptr<Lane>> (NUM_LANES));
        for(auto i = 0; i < NUM_LANES; i++)
            lanes->at(i) = shared_ptr<Lane>(new Lane);
        threads = shared_ptr<vector<shared_ptr<thread>>> (new vector<shared_ptr<thread>>(NUM_LANES));
        num_of_carts = 0;
    }
    
    shared_ptr<vector<shared_ptr<thread>>> get_threads(){return threads;}
    
    void start_checking_out(shared_ptr<Product_DataBase> table)
    {
        
        for(auto i = 0; i < NUM_LANES; i++)
        {
            threads->at(i) = shared_ptr<thread> (new thread(process_lanes, lanes->at(i), table, i + 1));
        }
        
        
        auto go = []()->void
        {
            std::unique_lock<std::mutex> lck(mtx);
            ready = true;
            cv.notify_all();
        };
        go();
        
        //for (auto& th : *threads) th->join();
    }

    
    void push_a_cart_into_a_lane(shared_ptr<Cart> newItem)
    {
        lanes->at(num_of_carts % NUM_LANES)->push_a_cart(newItem);
        num_of_carts ++;
    }
    
    void helper()
    {
        for(auto it = lanes->begin(); it != lanes->end(); it ++)
        {
            (*it)->helper();
        }
    }
    
};



class FileHandler2
{
private:
    string name;
    ifstream in;
public:
    FileHandler2()
    {
        name = "";
    }
    
    FileHandler2(string name)
    {
        this->name = name;
    }
    
    void setName(string name){this->name = name;}
    
    void openFile()
    {
        in.open(name);
        if (!in)
        {
            cerr<<"File cannot be opened\n";
            exit(EXIT_FAILURE);
        }
    }
    
    void readFile(CheckOutLanes &checkoutlanes, shared_ptr<Product_DataBase> table)
    {
        openFile();
        while(in)
        {
            string name;
            string line;
            getline(in, name);
            getline(in, line);
            shared_ptr<Cart> cart = shared_ptr<Cart> (new Cart);
            cart->set_id(name);
            cart->process_barcodes(line, table);
            checkoutlanes.push_a_cart_into_a_lane(cart);

        }
    }
    
};


int main(int argc, const char * argv[]) {
    // insert code here...
    
    
    shared_ptr<Product_DataBase> table = shared_ptr<Product_DataBase> (new Product_DataBase);
    FileHandler1 file1("ProductPrice.xml");
    file1.openFile(table);
    
    
    CheckOutLanes checkoutlanes;
    FileHandler2 file2("Carts.csv");
    file2.readFile(checkoutlanes, table);
    
    checkoutlanes.start_checking_out(table);
    
    
    shared_ptr<vector<shared_ptr<thread>>> threads = checkoutlanes.get_threads();
    for (auto& th : *threads) th->join();
    
    return 0;
}

