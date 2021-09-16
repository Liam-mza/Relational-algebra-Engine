/**********************************************
 *  Relational Algebra DBDM                   *
 *  Liam Mouzaoui, Catalin Moldovan           *
 *  École Normale Supérieure de Lyon          *
 *  Département Informatique - M1             *
 **********************************************/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include "include/json.hpp"
#include <boost/regex.hpp>
#include <thread>

using namespace std;
using namespace chrono;
using namespace boost;
using json = nlohmann::json;

// ------------ Global Variables -----------

string tableList[4] = { "departements", "employes", "membres", "projets" };

struct structure_str
{
    bool success = true;
    string errorMessage = "Invalid query sintax";

    string tableName; // Table Name // TO BE DELETED
    vector<string> attributes; // List of atributes
    map <string, vector<string>> records; // List of records for each attribute
};

struct condition_str
{
    string attribute;
    char op;
    string value;
    char log = ','; // default is ,
};

/// Just a structure that keeps information to be able to do parallelise operation
struct para_str
{
    structure_str *relation;
    structure_str *relation2;
    int startIndex;
    int nbToProcess;
    vector<condition_str> select;
    structure_str result;
};

/// Help the different thead to know what to do on the relation for the parrallel operations
vector<para_str> partitionTab;

enum operation { SELECT, PROJECT, CARTESIAN, RENAME, DIFFERENCE, UNION, ERROR };

bool optPC = 0;
bool optParallel = 0;

// ------------ Prototypes -----------
// -Functions-
operation hashit(string);
void show_result(structure_str);

// -CSV-
structure_str readFromCSV(string);

// -NOT REGEX-
string relation(string);
structure_str condition(string, vector<condition_str>*, char);
structure_str process_query(string);

// -REGEX-
structure_str relation_regex(string, string);
structure_str select_query_regex(string);
structure_str ucjd_query_regex(string, string);
structure_str rename_query_regex(string);
structure_str process_query_regex(string);
structure_str project_cartesian_query_regex(string);
structure_str jpr_query_regex(string);

// -C++ Operations-
bool is_number(string);
int haveSameAttributs(vector<string>,vector<string>);
int existeAndIndex_attribute(vector<string>, string);
bool testAnEntry(string, condition_str);
bool check_cond(structure_str*, structure_str*, int, int, vector<condition_str>);

// -JSON-
string readJSONfile(string);
string queryJSON(json);
string conditionJSON(json, bool);
string attributeJSON(json);

//-QUERY FUNCTIONS-
structure_str do_select_query( structure_str*, condition_str);
structure_str do_renaming(structure_str*, string, string);
structure_str do_project(structure_str*, vector<string>);
structure_str do_minus(structure_str*, structure_str*);
structure_str do_union(structure_str*, structure_str*);
structure_str do_cartesianProduc(structure_str*, structure_str*);
structure_str joinProjectRename(structure_str*, structure_str*, vector<string>, vector<string>, vector<string>, vector<condition_str>);

// -PARALLELIZATION-
structure_str joinProjectRename_parra(structure_str*, structure_str*, vector<string>, vector<string>, vector<string>, vector<condition_str>);
structure_str do_cartesianProductParrallele(structure_str*, structure_str*);

// -TEST-
void computeTest();

// ------------ Main -----------

int main()
{
    cout << "1. Selection:        SELECT[condition](relation)\n"; // The relation is the name of the table from the database
    // (EXAMPLES) SELECT[ide<3](employes) ; SELECT[ide<3|nom='Hamish Fulton'](employes) ; SELECT[ide>52,email!'luctus@bibendum.com'](employes)
    cout << "2. Projection:       PROJECT[attribute](relation)\n";
    cout << "3. Cartesian:        CARTESIAN(relation_1)(relation_2)\n";
    cout << "4. Renaiming:        RENAME[new_attribute_name(attribute)](relation)\n";
    cout << "5. Minus/Difference: DIFFERENCE(relation_1)(relation_2)\n";
    cout << "6. Union:            UNION(relation_1)(relation_2)\n";
    cout << "7. Jpr:              JPR[new_attribute_name(attribute)][condition](relation)(relation)\n\n";

    cout << "*For optimized functions type: 'enable OPT' or 'disable OPT' (default: ENABLED)\n";
    cout << "**To load the JSON file type: `load`\n";
    cout << "***To launch the Benchmark type: `test`";
    cout << "\n\n\n";

    // Start of the application
    string query = "";
    while (true)
    {
        cout << "query> ";
        getline(cin, query);
        if (query == "exit")
            break;
        else if (query == "")
            continue;
        else if (query == "enable OPT")
        {
            cout << "> Optimized Cartesian Functions ENABLED\n";
            optPC = 1;
        }
        else if (query == "disable OPT")
        {
            cout << "> Optimized Cartesian Functions DISABLED\n";
            optPC = 0;
        }
        else if (query == "enable PARA")
        {
            cout << "> Parallel Functions ENABLED\n";
            optParallel = 1;
        }
        else if (query == "disable PARA")
        {
            cout << "> Parallel Functions DISABLED\n";
            optParallel = 0;
        }
        else if (query == "test")
            computeTest();
        else
        {
            structure_str output;
            if (query == "load") // Load JSON file
                query = readJSONfile("samples/query.json"); // Convert from JSON to RA

            auto start = high_resolution_clock::now(); // Start Execution Time
            output = process_query_regex(query); // Process Query Regex
            auto stop = high_resolution_clock::now();  // End Execution Time
            auto executionTime = duration_cast<microseconds>(stop - start);

            if (output.success)
            {
                show_result(output); // Show results on screen
                cout << "Execution Time: " << executionTime.count() << " microseconds\n\n";
            }
            else
               cout << "ERROR: " + output.errorMessage + "!\n\n";
        }
    }

    return 0;
}

void computeTest()
{
    structure_str dataEx;
    dataEx = readFromCSV("employes");
    if (!dataEx.success) {
        cout << "impossible de lire le fichier employes";
    }

    structure_str dataEx2;
    dataEx2 = readFromCSV("projets");
    if (!dataEx2.success) {
        cout << "impossible de lire le fichier projets";
    }

    vector<string> proj;
    proj.push_back("ide");
    proj.push_back("nom");
    proj.push_back("titre");
    proj.push_back("idp");

    vector<string> old;
    old.push_back("ide");
    old.push_back("nom");
    old.push_back("titre");

    vector<string> newName;
    newName.push_back("id");
    newName.push_back("Nom Prenom");
    newName.push_back("intitulé");

    vector<condition_str> condition;
    condition_str cond;
    cond.attribute = "ide";
    cond.op = '<';
    cond.value = "80";
    cond.log = '|';
    condition.push_back(cond);

    //TEST CARTESIAN PRODUCT
    auto start = high_resolution_clock::now(); // Start Execution Time
    structure_str output = do_cartesianProduc(&dataEx, &dataEx2);
    auto stop = high_resolution_clock::now();  // End Execution Time
    auto executionTime = duration_cast<microseconds>(stop - start);
    auto start2 = high_resolution_clock::now(); // Start Execution Time
    output = do_cartesianProductParrallele(&dataEx, &dataEx2);
    auto stop2 = high_resolution_clock::now();  // End Execution Time
    auto executionTime2 = duration_cast<microseconds>(stop2 - start2);

    cout << "TEST OF CARTESIAN PRODUCT \n    -> Without optimisation: " << executionTime.count() << " microseconds\n    -> With optimisation(PART 3): " << executionTime2.count() << " microseconds\n";

    //TEST JOIN PROJECT RENAME
    start = high_resolution_clock::now(); // Start Execution Time
    output = do_cartesianProduc(&dataEx, &dataEx2);
    output = do_select_query(&output, cond);
    output = do_project(&output, proj);
    output = do_renaming(&output, "nom", "Nom Prenom");
    stop = high_resolution_clock::now();  // End Execution Time
    executionTime = duration_cast<microseconds>(stop - start);
    start2 = high_resolution_clock::now(); // Start Execution Time
    output = joinProjectRename(&dataEx, &dataEx2, proj, old, newName, condition);
    stop2 = high_resolution_clock::now();  // End Execution Time
    executionTime2 = duration_cast<microseconds>(stop2 - start2);
    auto start3 = high_resolution_clock::now(); // Start Execution Time
    output = joinProjectRename_parra(&dataEx, &dataEx2, proj, old, newName, condition);
    auto stop3 = high_resolution_clock::now();  // End Execution Time
    auto executionTime3 = duration_cast<microseconds>(stop3 - start3);

    cout << "TEST OF JOIN PROJECT RENAME \n    -> Basic interpretor: " << executionTime.count() << " microseconds\n    -> With optimisation(PART 2 ONLY): " << executionTime2.count() << " microseconds\n    -> With full optimisation(PART 3 included): " << executionTime3.count() << " microseconds\n";

    bool currentOPTPC = optPC;
    bool currentOPTParallel = optParallel;
    string query = readJSONfile("samples/query.json"); // Convert from JSON to RA

    // Without OPT
    optPC = 0;
    currentOPTParallel = 0;
    start = high_resolution_clock::now(); // Start Execution Time
    output = process_query_regex(query); // Process Query Regex
    stop = high_resolution_clock::now();  // End Execution Time
    executionTime = duration_cast<microseconds>(stop - start);
    optPC = 1;
    start2 = high_resolution_clock::now(); // Start Execution Time
    output = process_query_regex(query); // Process Query Regex
    stop2 = high_resolution_clock::now();  // End Execution Time
    executionTime2 = duration_cast<microseconds>(stop2 - start2);
    currentOPTParallel = 1;
    start3 = high_resolution_clock::now(); // Start Execution Time
    output = process_query_regex(query); // Process Query Regex
    stop3 = high_resolution_clock::now();  // End Execution Time
    executionTime3 = duration_cast<microseconds>(stop3 - start3);
    cout << "LOAD FROM JSON FILE \n    -> Basic interpretor: " << executionTime.count()  << " microseconds\n    -> With optimisation(PART 2 ONLY): " << executionTime2.count() << " microseconds\n    -> With full optimisation(PART 3 included): " << executionTime3.count() << " microseconds\n";

    optPC = currentOPTPC;
    optParallel = currentOPTParallel;
}

// ------------ Functions -----------

operation hashit(string inOperation)
{
    if (inOperation == "SELECT") return SELECT;
    if (inOperation == "PROJECT") return PROJECT;
    if (inOperation == "CARTESIAN") return CARTESIAN;
    if (inOperation == "RENAME") return RENAME;
    if (inOperation == "DIFFERENCE") return DIFFERENCE;
    if (inOperation == "UNION") return UNION;
    return ERROR;
}

// Show result table on Terminal
void show_result(structure_str data)
{
    unsigned int maxLineLength = 0;
    unsigned int* maxRowsLength = new unsigned int[data.attributes.size()];

    unsigned int columns = data.attributes.size();
    unsigned int rows = data.records[data.attributes[0]].size();
    
    // Set max length for each column
    for (int i = 0; i < columns; ++i)
    {
        maxRowsLength[i] = data.attributes[i].length();
        for (int j = 0; j < rows; ++j)
            if (data.records[data.attributes[i]][j].length() > maxRowsLength[i])
                maxRowsLength[i] = data.records[data.attributes[i]][j].length();
        maxLineLength += maxRowsLength[i] + 3;
    }

    // Show attributes
    cout << "\n ";
    for (int i = 0; i < maxLineLength + 1; ++i)
        cout << "=";
    cout << "\n | ";
    for (int i = 0; i < columns; ++i)
    {
        cout << data.attributes[i];
        for (int k = 0; k < maxRowsLength[i] - data.attributes[i].length(); ++k)
            cout << " ";
        cout << " | ";
    }
    cout << "\n ";
    for (int i = 0; i < maxLineLength + 1; ++i)
        cout << "=";
    cout << "\n ";

    // Show records
    for (int j = 0; j < rows; ++j)
    {
        cout << "| ";
        for (int i = 0; i < columns; ++i)
        {
            cout << data.records[data.attributes[i]][j];
            for (int k = 0; k < maxRowsLength[i] - data.records[data.attributes[i]][j].length(); ++k)
                cout << " ";
            cout << " | ";
        }
        cout << "\n ";
    }
    for (int i = 0; i < maxLineLength + 1; ++i)
        cout << "-";

    cout << "\nShowing " << rows << " rows";
    cout << "\n\n";
}

structure_str readFromCSV(string fileName)
{
    structure_str output;

    ifstream csvFile("samples/" + fileName + ".csv");
    if (!csvFile.good())
    {
        output.success = false;
        output.errorMessage = "An error occured while opening the csv file";
        return output;
    }

    // Get the attributes list
    string attLine;
    getline(csvFile, attLine); // First line of CSV file represents the attributes line
    unsigned int noAtt = 0;
    for (int i = 0; i <= noAtt; ++i)
    {
        size_t attSymbol = attLine.find(',');
        if (attSymbol != std::string::npos) // If it's not the end of the attributes line, then increment the att counter, or else add the last attribute to the list
            noAtt++;
        output.attributes.push_back(attLine.substr(0, attSymbol)); // Add to attributes list
        attLine = attLine.substr(attSymbol + 1, attLine.length()); // Substring the attributes line
    }

    // Get the records list
    string recordCell;
    unsigned int k = -1; // Counter
    while (true)
    {
        if (++k % output.attributes.size() == output.attributes.size() - 1) // That's the record for the last attribute which has a newline at the end and not a ,
        {
            if (!getline(csvFile, recordCell, '\n'))
                break;
        }
        else
            if (!getline(csvFile, recordCell, ','))
                break;

        // Add record
        output.records[output.attributes[k % output.attributes.size()]].push_back(recordCell);
    }

    return output;
}

bool is_number(string str)
{
    string::const_iterator iterator = str.begin();
    while (iterator != str.end() && isdigit(*iterator)) // Iterate over the string until we find a non-digit character
        ++iterator;
    return !str.empty() && iterator == str.end();
}
int existeAndIndex_attribute(vector<string> attributes, string name) {
    for (int i = 0; i < attributes.size(); ++i) {
        if (attributes[i].compare(name) == 0) {
            return i;
        }
    }
    return -1;
}
int haveSameAttributs(vector<string> att1,vector<string> att2){
    for (auto & categorie : att1) {
        if(existeAndIndex_attribute(att2, categorie) !=-1){
            return 0;
        }
    }
    return 1;
}

/**
 Test if the given element satisfy the condition
 @param element : the element to test
 @param condition : the condition to satisfy
 @return bool : the result of the test
 */
bool testAnEntry(string element, condition_str condition) {

    if (condition.op == '<') {
        return (is_number(element) && (stoi(element) < stoi(condition.value)));
    }
    else if (condition.op == '=') {
        return (element.compare(condition.value) == 0);

    }
    else if (condition.op == '>') {
        return (is_number(element) && (stoi(element) > stoi(condition.value)));
    }
    else if (condition.op == '!') {
        return (!(element.compare(condition.value) == 0));
    }
    else if (condition.op == '}') {
        return ((is_number(element) && (stoi(element) >= stoi(condition.value))) || (element.compare(condition.value) == 0));
    }
    else if (condition.op == '{') {
        return((is_number(element) && (stoi(element) <= stoi(condition.value))) || (element.compare(condition.value) == 0));
    }
    else {
        return false;
    }
}

/// check that element i of the first relation and element j of the second one satisfy the given condition
/// @param relation : the first relation to test
/// @param relation2 : the second relation to test
/// @param i :  index of the element to test from relation
/// @param j : index of the element to test from relation2
/// @param condition : the condition to satisfy
/// @return bool : the result of the test
bool check_cond(structure_str* relation, structure_str* relation2, int i, int j, vector<condition_str> condition) {
    char log;
    bool res;
    bool start = true;

    for (auto& cond : condition) {
        int selectRelation = 0;
        bool test = false;

        if (existeAndIndex_attribute(relation->attributes, cond.attribute) != -1) {
            selectRelation = 1;
        }
        if (selectRelation == 0 && existeAndIndex_attribute(relation2->attributes, cond.attribute) != -1) {
            selectRelation = 2;
        }
        if (selectRelation == 0) {
            return true;
        }

        if (selectRelation == 1) {
            test = testAnEntry((relation->records[cond.attribute])[i], cond);
        }
        if (selectRelation == 2) {
            test = testAnEntry((relation2->records[cond.attribute])[j], cond);
        }

        if (start) {
            res = test;
            start = false;
            log = cond.log;
        }
        else {
            if (log == ',') {
                res = res && test;
            }
            else if (log == '|') {
                res = res || test;
            }
            log = cond.log;
        }
    }
    return res;
}

/**
 Compute a selection on the given relation with respect to the given condition
 @param relation : the relation on which we want to do the selection
 @param condition : the condition to do the selection
 @return structure_str result:  the result relation of the selection
 */
structure_str do_select_query( structure_str* relation, condition_str condition){
    structure_str result;
    result.tableName="Select result";
    vector<string> attributes = relation->attributes;
    map <string, vector<string> > records;
    for (auto & categorie : attributes){
        vector<string> t;
        records.insert(std::pair<string,vector<string> >(categorie, t));
    }
    int index=existeAndIndex_attribute(attributes, condition.attribute)==-1;
    if(index==-1){
        result.success=false;
        cout <<"attribut error \n";
        result.errorMessage="attribut du select n'existe pas";
        return result;
    }
    
    //Test de l'opŽrateur
    if (condition.op == '<'){
        int i=0;
        for (auto & element : relation->records.at(condition.attribute)){
            
            if (is_number(element) && (stoi(element)<stoi(condition.value))){
                for (auto & categorie : attributes){
                    records.at(categorie).push_back(relation->records.at(categorie)[i]);
                }
            }
            ++i;
        }
    } else if (condition.op == '='){
        int i=0;
        for (auto & element : relation->records.at(condition.attribute)){
            if (element.compare(condition.value)==0){
                for (auto & categorie : attributes){
                    records.at(categorie).push_back(relation->records.at(categorie)[i]);
                }
            }
            ++i;
        }
        
    } else if (condition.op == '>'){
        int i=0;
        for (auto & element : relation->records.at(condition.attribute)){
            if (is_number(element) && (stoi(element)>stoi(condition.value))){
                for (auto & categorie : attributes){
                    records.at(categorie).push_back(relation->records.at(categorie)[i]);
                }
            }
            ++i;
        }
    }
    else if (condition.op == '!') {
        int i = 0;
        for (auto& element : relation->records.at(condition.attribute)) {
            if (!(element.compare(condition.value) == 0)) {
                for (auto& categorie : attributes) {
                    records.at(categorie).push_back(relation->records.at(categorie)[i]);
                }
            }
            ++i;
        }
    }
    else if (condition.op == '}') {
        int i = 0;
        for (auto& element : relation->records.at(condition.attribute)) {
            if ((is_number(element) && (stoi(element) >= stoi(condition.value))) || (element.compare(condition.value) == 0)) {
                for (auto& categorie : attributes) {
                    records.at(categorie).push_back(relation->records.at(categorie)[i]);
                }
            }
            ++i;
        }
    }
    else if (condition.op == '{') {
        int i = 0;
        for (auto& element : relation->records.at(condition.attribute)) {

            if ((is_number(element) && (stoi(element) <= stoi(condition.value))) || (element.compare(condition.value) == 0)) {
                for (auto& categorie : attributes) {
                    records.at(categorie).push_back(relation->records.at(categorie)[i]);
                }
            }
            ++i;
        }
    }
    else{
        result.success=false;
        cout <<"op error \n";
        result.errorMessage="attribut du select n'existe pas";
        return result;
    }
    
    result.attributes=attributes;
    result.records=records;
    result.success=true;
    return result;
}

/**
 Do a renaming on the given relation
 @param relation : the relation where we want to do the renaming
 @param attributeName : the attribut to rename
 @param value : the new name of the attribute
 @return structure_str : result relation of the renaming
 */
structure_str do_renaming(structure_str* relation, string attributeName, string value) {
    structure_str result;
    result.tableName = "Renaming result";
    result.attributes = relation->attributes;
    result.records = relation->records;
    for (int i = 0; i < result.attributes.size(); ++i) {
        if (result.attributes[i].compare(attributeName) == 0) {
            result.attributes[i] = value;
            vector<string> tmp = result.records.at(attributeName);
            result.records.insert(std::pair<string, vector<string> >(value, tmp));
            result.records.erase(attributeName);
            result.success = true;
            return result;
        }
    }
    result.success = false;
    cout << "Renaming error \n";
    result.errorMessage = "attribut du renaming n'existe pas";
    return result;
}

/**
 Do a projection on the given relation
 @param relation : the relation we want to do the projection on
 @param value : the attributes we want to keep
 @return structure_str : the relation after the projection
 */
structure_str do_project(structure_str* relation, vector<string> value)
{
    structure_str result;
    result.tableName = "project result";
    result.records = relation->records;

    for (int i = 0; i < relation->attributes.size(); ++i) {
        if (existeAndIndex_attribute(value, relation->attributes[i]) == -1) {
            result.records.erase(relation->attributes[i]);
        }
        else {
            result.attributes.push_back(relation->attributes[i]);
        }
    }


    result.success = true;
    return result;
}

/**
 Do the difference between the first relation and the second one
 @param relation : the first relation
 @param relation2 : the second relation, the one to substract
 @return structure_str : the result relation of the minus
 */
structure_str do_minus(structure_str* relation, structure_str* relation2) {
    if (relation2->attributes.size() != relation->attributes.size()) {
        structure_str result;
        result.success = false;
        result.tableName = "Minus result";
        result.errorMessage = "les deux relation n'ont pas le mme nombre d'attributs";
        return result;
    }
    if (relation2->attributes.size() == 0 || relation2->records[relation2->attributes[0]].size() == 0) {
        structure_str result;
        result.success = true;
        result.tableName = "Minus result";
        result.attributes = relation->attributes;
        result.records = relation->records;
        return result;
    }
    if (relation->attributes.size() == 0 || relation->records[relation->attributes[0]].size() == 0) {
        structure_str result;
        result.success = true;
        result.tableName = "Minus result";
        result.attributes = relation->attributes;
        result.records = relation->records;
        return result;
    }
    structure_str result;
    result.tableName = "Minus result";
    result.attributes = relation->attributes;
    result.records = relation->records;
    int erased = 0;
    int size = relation->records[relation->attributes[0]].size();
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < relation2->records[relation2->attributes[0]].size(); ++j) {
            bool find = true;
            for (auto& categorie : relation->attributes) {
                string t1 = relation->records.at(categorie)[i];
                string t2 = relation2->records.at(categorie)[j];
                if (relation->records.at(categorie)[i].compare(relation2->records.at(categorie)[j]) != 0) {
                    find = false;
                    break;
                }
            }
            if (find) {
                for (auto& categorie : relation->attributes) {
                    result.records[categorie].erase(result.records[categorie].begin() + i - erased);
                }
                ++erased;
            }
        }
    }
    result.success = true;
    return result;
}

/**
 Compute the union of the two given relations
 @param relation : the first relation
 @param relation2 : the second relation
 @return structure_str : the result relation of the union
 */
structure_str do_union(structure_str* relation, structure_str* relation2){

    if(relation2->attributes.size()!=relation->attributes.size()){
        structure_str result;
        result.success=false;
        result.tableName="Union result";
        result.errorMessage="les deux relation n'ont pas le mme nombre d'attributs";
        return result;
    }
    if (relation2->attributes.size()==0 || relation2->records[relation2->attributes[0]].size()==0){
        structure_str result;
        result.success=true;
        result.tableName="Union result";
        result.attributes = relation->attributes;
        result.records=relation->records;
        return result;
    }
    if (relation->attributes.size()==0 || relation->records[relation->attributes[0]].size()==0){
        structure_str result;
        result.success=true;
        result.tableName="Union result";
        result.attributes = relation2->attributes;
        result.records=relation2->records;
        return result;
    }

    structure_str result;
    result.tableName="Union result";
    result.attributes = relation->attributes;
    result.records=relation->records;
    
    
    for (int i=0; i<relation2->records[relation2->attributes[0]].size(); ++i) {
        bool find =false;
        for (int j=0; j<relation->records[relation->attributes[0]].size(); ++j) {
            int match=0;
            for (auto & categorie : relation->attributes){
                string t1=relation2->records.at(categorie)[i];
                string t2=relation->records.at(categorie)[j];
                if(relation2->records.at(categorie)[i].compare(relation->records.at(categorie)[j])==0){
                    ++match;
                    if (match==relation->attributes.size()){
                        find=true;
                    }
                }
            }
            if(find){
                break;
            }
        }
        if (!find){
            for (auto & categorie : relation->attributes){
                result.records[categorie].push_back(relation2->records.at(categorie)[i]);
            }
        }
        find=false;
    }
    
    
    result.success=true;
    return result;
}

/**
 Compute the cartesian product of the two given relations
 @param relation : the first relation
 @param relation2 : the second relation
 @return structure_str : the result relation of the cartesian product
 */
structure_str do_cartesianProduc(structure_str* relation, structure_str* relation2){
    if(relation==NULL || relation2==NULL){
        structure_str result;
        result.success=false;
        result.tableName="Cartesian product result";
        result.errorMessage="un des pointeurs est null";
        return result;
    }
    if (relation2->attributes.size()==0 || relation2->records[relation2->attributes[0]].size()==0){
        structure_str result;
        result.success=true;
        result.tableName="Cartesian product result";
        result.attributes = relation->attributes;
        result.records=relation->records;
        return result;
    }
    if (relation->attributes.size()==0 || relation->records[relation->attributes[0]].size()==0){
        structure_str result;
        result.success=true;
        result.tableName="Cartesian product result";
        result.attributes = relation2->attributes;
        result.records=relation2->records;
        return result;
    }

    structure_str tmp = *relation2;
    for (auto& categorie : relation2->attributes) {
        if (existeAndIndex_attribute(relation->attributes, categorie) != -1) {
            tmp = do_renaming(relation2, categorie, categorie + "2");
        }
    }
    relation2 = &tmp;

    structure_str result;
    result.tableName="Cartesian product result";
    result.attributes = relation->attributes;
    for (auto & categorie : relation2->attributes){
        result.attributes.push_back(categorie);
    }
    
    for (int i=0; i<relation->records[relation->attributes[0]].size(); ++i) {
        for (int j=0; j<relation2->records[relation2->attributes[0]].size(); ++j) {
            if (i==0 && j==0){
                for (auto & categorie : relation->attributes){
                    vector<string> t;
                    t.push_back((relation->records[categorie])[i]);
                    result.records.insert(std::pair<string,vector<string> >(categorie, t)  );
                }
                for (auto & categorie : relation2->attributes){
                    vector<string> t;
                    t.push_back((relation2->records[categorie])[j]);
                    result.records.insert(std::pair<string,vector<string> >(categorie, t)  );
                }
            }
            else{
                for (auto & categorie : relation->attributes){
                    result.records[categorie].push_back((relation->records[categorie])[i]);
                }
                for (auto & categorie : relation2->attributes){
                    result.records[categorie].push_back((relation2->records[categorie])[j]);
                }
            }
            
        }
    }
    
    
    result.success=true;
    return result;
}

/**
 Auxiliary function to make the parallelisation of the cartesian product
 @param index : just an index to dertermine wich part of the relation the thread take care of
 */
void aux_para_cartesian(int index){
    structure_str result;
    result.tableName="Cartesian product result";
    result.attributes = partitionTab[index].relation->attributes;
    for (auto & categorie : partitionTab[index].relation2->attributes){
            result.attributes.push_back(categorie);
    }
    
    for (int i=partitionTab[index].startIndex; i<partitionTab[index].startIndex+partitionTab[index].nbToProcess; ++i) {
        for (int j=0; j<partitionTab[index].relation2->records[partitionTab[index].relation2->attributes[0]].size(); ++j) {
            if (i==0 && j==0){
                for (auto & categorie : partitionTab[index].relation->attributes){
                    vector<string> t;
                    t.push_back((partitionTab[index].relation->records[categorie])[i]);
                    result.records.insert(std::pair<string,vector<string> >(categorie, t)  );
                }
                for (auto & categorie : partitionTab[index].relation2->attributes){
                    vector<string> t;
                    t.push_back((partitionTab[index].relation2->records[categorie])[j]);
                    result.records.insert(std::pair<string,vector<string> >(categorie, t)  );
                }
            }
            else{
                for (auto & categorie : partitionTab[index].relation->attributes){
                    result.records[categorie].push_back((partitionTab[index].relation->records[categorie])[i]);
                }
                for (auto & categorie : partitionTab[index].relation2->attributes){
                    result.records[categorie].push_back((partitionTab[index].relation2->records[categorie])[j]);
                }
            }
            
        }
    }
    partitionTab[index].result=result;
    
}

/**
 Compute the cartesian product of the two given relations but in parallel
 @param relation : the first relation
 @param relation2 : the second relation
 @return structure_str : the result relation of the cartesian product
 */
structure_str do_cartesianProductParrallele(structure_str* relation, structure_str* relation2){
    if(relation==NULL || relation2==NULL){
        structure_str result;
        result.success=false;
        result.tableName="Cartesian product result";
        result.errorMessage="un des pointeurs est null";
        return result;
    }
    
    if (relation2->attributes.size()==0 || relation2->records[relation2->attributes[0]].size()==0){
        structure_str result;
        result.success=true;
        result.tableName="Cartesian product result";
        result.attributes = relation->attributes;
        result.records=relation->records;
        return result;
    }
    if (relation->attributes.size()==0 || relation->records[relation->attributes[0]].size()==0){
        structure_str result;
        result.success=true;
        result.tableName="Cartesian product result";
        result.attributes = relation2->attributes;
        result.records=relation2->records;
        return result;
    }
    
    structure_str tmp= *relation2;
    for (auto & categorie : relation2->attributes){
        if (existeAndIndex_attribute(relation->attributes, categorie)!=-1) {
            tmp = do_renaming(relation2, categorie, categorie+"2");
        }
    }
    relation2= &tmp;
    
    structure_str result;
    result.tableName="Cartesian product result";
    result.attributes = relation->attributes;
    for (auto & categorie : relation2->attributes){
            result.attributes.push_back(categorie);
    }
    
    int size = relation->records[relation->attributes[0]].size();
    
   
    for (int i=0; i<4; ++i) {
        para_str t;
        t.relation=relation;
        t.relation2=relation2;
        t.startIndex= i*(size/4);
        if (i==3) {
            t.nbToProcess=size-3*(size/4);
        }else{
            t.nbToProcess=size/4;
        }
        partitionTab.push_back(t);
    }
    thread thread1(aux_para_cartesian,0);
    thread thread2(aux_para_cartesian,1);
    thread thread3(aux_para_cartesian,2);
    thread thread4(aux_para_cartesian,3);
   
    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();
    
    for (int i=0; i<4; ++i) {
        for (auto & categorie : result.attributes){
            result.records[categorie].insert(result.records[categorie].end(), partitionTab[i].result.records[categorie].begin(),  partitionTab[i].result.records[categorie].end());
        }
    }
    
    for (int i = 0; i < 4; ++i) {
        partitionTab.pop_back();
    }

    
    result.success=true;
    return result;
}

/// Compute the operation JoinProjectRename from part 2.1
/// @param relation : the first relation from the join
/// @param relation2 : the second relation from the join
/// @param projection : the projection to apply after the join
/// @param renameAttribute : attrinuts to rename to apply after the projection
/// @param newName : new name for the renamed attributs
/// @param select : the conditions for the join
/// @return structure_str: the result of the JoinProjectRename operation
structure_str joinProjectRename(structure_str* relation, structure_str* relation2, vector<string> projection, vector<string> renameAttribute, vector<string> newName, vector<condition_str> select) {
    if (relation == NULL || relation2 == NULL) {
        structure_str result;
        result.success = false;
        result.tableName = "JoinProjectRename result";
        result.errorMessage = "un des pointeurs est null";
        return result;
    }

    structure_str tmp = *relation2;
    for (auto& categorie : relation2->attributes) {
        if (existeAndIndex_attribute(relation->attributes, categorie) != -1) {
            tmp = do_renaming(relation2, categorie, categorie + "2");
        }
    }
    relation2 = &tmp;
    bool only1 = true;
    for (auto& cond : select) {
        if (existeAndIndex_attribute(relation2->attributes, cond.attribute) != -1) {
            only1 = false;
        }
    }

    structure_str result;
    result.tableName = "JoinProjectRename result";
    result.attributes = relation->attributes;
    for (auto& categorie : relation2->attributes) {
        result.attributes.push_back(categorie);
        vector<string> t;
        result.records.insert(std::pair<string, vector<string> >(categorie, t));
    }
    for (auto& categorie : relation->attributes) {
        vector<string> t;
        result.records.insert(std::pair<string, vector<string> >(categorie, t));
    }

    for (int i = 0; i < relation->records[relation->attributes[0]].size(); ++i) {
        if (only1) {
            if (!check_cond(relation, relation2, i, 0, select)) {
                continue;
            }
        }
        for (int j = 0; j < relation2->records[relation2->attributes[0]].size(); ++j) {
            if (!only1 && !check_cond(relation, relation2, i, j, select)) {
                continue;
            }
            for (auto& categorie : relation->attributes) {
                result.records[categorie].push_back((relation->records[categorie])[i]);
            }
            for (auto& categorie : relation2->attributes) {
                result.records[categorie].push_back((relation2->records[categorie])[j]);
            }
        }
    }

    vector<string> tmp2 = result.attributes;

    for (int i = 0; i < tmp2.size(); ++i) {
        if (existeAndIndex_attribute(projection, tmp2[i]) == -1) {
            result.records.erase(tmp2[i]);
        }
    }
    result.attributes = projection;

    for (int i = 0; i < renameAttribute.size(); ++i) {
        int index = existeAndIndex_attribute(result.attributes, renameAttribute[i]);
        if (index != -1) {
            result.attributes[index] = newName[i];
            vector<string> tmp = result.records.at(renameAttribute[i]);
            result.records.insert(std::pair<string, vector<string> >(newName[i], tmp));
            result.records.erase(renameAttribute[i]);
        }
    }
    result.success = true;
    return result;
}

/**
 Auxiliary function to make the parallelisation of the jpr operation
 @param index : just an index to dertermine wich part of the relation the thread take care of
 */
void aux_para_jpr(int index){
    bool only1=true;
    for (auto & cond : partitionTab[index].select){
        if (existeAndIndex_attribute(partitionTab[index].relation2->attributes, cond.attribute)!=-1) {
            only1=false;
        }
    }
    
    structure_str result;
    result.tableName="JoinProjectRename result";
    result.attributes = partitionTab[index].relation->attributes;
    for (auto & categorie : partitionTab[index].relation2->attributes){
        result.attributes.push_back(categorie);
        vector<string> t;
        result.records.insert(std::pair<string,vector<string> >(categorie, t)  );
    }
    for (auto & categorie : partitionTab[index].relation->attributes){
        vector<string> t;
        result.records.insert(std::pair<string,vector<string> >(categorie, t)  );
    }
    
    for (int i=partitionTab[index].startIndex; i<partitionTab[index].startIndex+partitionTab[index].nbToProcess; ++i) {
        if (only1){
            if (!check_cond(partitionTab[index].relation, partitionTab[index].relation2, i, 0, partitionTab[index].select)){
                continue;
            }
        }
        for (int j=0; j<partitionTab[index].relation2->records[partitionTab[index].relation2->attributes[0]].size(); ++j) {
            if (!only1 && !check_cond(partitionTab[index].relation, partitionTab[index].relation2, i, j, partitionTab[index].select)){
                continue;
            }
                for (auto & categorie : partitionTab[index].relation->attributes){
                    result.records[categorie].push_back((partitionTab[index].relation->records[categorie])[i]);
                }
                for (auto & categorie : partitionTab[index].relation2->attributes){
                    result.records[categorie].push_back((partitionTab[index].relation2->records[categorie])[j]);
                }
        }
    }
    partitionTab[index].result=result;
}

/// Compute the operation JoinProjectRename from part 2.1 but in parallel
/// @param relation : the first relation from the join
/// @param relation2 : the second relation from the join
/// @param projection : the projection to apply after the join
/// @param renameAttribute : attrinuts to rename to apply after the projection
/// @param newName : new name for the renamed attributs
/// @param select : the conditions for the join
/// @return structure_str: the result of the JoinProjectRename operation
structure_str joinProjectRename_parra(structure_str* relation, structure_str* relation2, vector<string> projection, vector<string> renameAttribute, vector<string> newName, vector<condition_str> select){
    if(relation==NULL || relation2==NULL){
        structure_str result;
        result.success=false;
        result.tableName="JoinProjectRename result";
        result.errorMessage="un des pointeurs est null";
        return result;
    }
    
    structure_str tmp= *relation2;
    for (auto & categorie : relation2->attributes){
        if (existeAndIndex_attribute(relation->attributes, categorie)!=-1) {
            tmp = do_renaming(relation2, categorie, categorie+"2");
        }
    }
    relation2= &tmp;
    
    structure_str result;
    result.tableName="JoinProjectRename result";
    result.attributes = relation->attributes;
    
    
    int size = relation->records[relation->attributes[0]].size();
    
   
    for (int i=0; i<4; ++i) {
        para_str t;
        t.relation=relation;
        t.relation2=relation2;
        t.startIndex= i*(size/4);
        if (i==3) {
            t.nbToProcess=size-3*(size/4);
        }else{
            t.nbToProcess=size/4;
        }
        t.select=select;
        partitionTab.push_back(t);
    }
    thread thread1(aux_para_jpr,0);
    thread thread2(aux_para_jpr,1);
    thread thread3(aux_para_jpr,2);
    thread thread4(aux_para_jpr,3);
   
    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();
    
    result.attributes=partitionTab[0].result.attributes;
    for (int i=0; i<4; ++i) {
        for (auto & categorie : result.attributes){
            result.records[categorie].insert(result.records[categorie].end(), partitionTab[i].result.records[categorie].begin(),  partitionTab[i].result.records[categorie].end());
        }
    }
    
    vector<string> tmp2 = result.attributes;
    
    for (int i=0; i< tmp2.size(); ++i) {
        if (existeAndIndex_attribute(projection, tmp2[i])==-1){
            result.records.erase(tmp2[i]);
        }
    }
    result.attributes=projection;
    
    for (int i=0; i< renameAttribute.size(); ++i) {
        int index =existeAndIndex_attribute(result.attributes, renameAttribute[i]);
        if(index!=-1){
            result.attributes[index]= newName[i];
            vector<string> tmp= result.records.at(renameAttribute[i]);
            result.records.insert(std::pair<string,vector<string> >(newName[i], tmp));
            result.records.erase(renameAttribute[i]);
        }
    }

    for (int i = 0; i < 4; ++i) {
        partitionTab.pop_back();
    }


    result.success=true;
    return result;
}

// (relation)
string relation(string query)
{
    size_t endSymbol = query.find(')'); // Stop the seraching at the first ')' symbol
    if (query[0] == '(' && endSymbol != std::string::npos && endSymbol == query.length() - 1)
        return query.substr(1, endSymbol - 1);
    return "";
}

// [condition]
structure_str condition(string fullCondition, vector<condition_str>* conditions, char log_op = ',') // default logical oeprator of condition is , (AND)
{
    structure_str output;
    string myCondition;

    size_t log = fullCondition.find_first_of(",|"); // Stop the searching at the first ',!' symbols
    if (log != std::string::npos)
    {
        myCondition = fullCondition.substr(0, log); // Take just the first condition, used outside this condition
        output = condition(fullCondition.substr(log + 1, fullCondition.length()), conditions, fullCondition.substr(log, log + 1)[0]); // Recursive for the next condition, and $ -> logical operator
    }
    else
        myCondition = fullCondition;

    size_t op = myCondition.find_first_of("><=!"); // Stop the seraching at the first '><=!' symbols
    if (op != std::string::npos)
    {
        string att = myCondition.substr(0, op); // (attribute_1) - Get he text before the '><=!' symbol

        if (isdigit(myCondition.substr(op + 1, myCondition.length())[0]) || myCondition.substr(op + 1, myCondition.length())[0] == '\'') // Check if the operator starts with a digit or ' symbol
        {
            string value;
            if (isdigit(myCondition.substr(op + 1, myCondition.length())[0])) // First letter of value
                value = myCondition.substr(op + 1, myCondition.length());
            else if (myCondition.substr(op + 1, myCondition.length())[0] == '\'')
            {
                size_t endValue = myCondition.substr(op + 2, myCondition.length()).find('\''); // Find the first ' at the end of the value
                if (endValue != std::string::npos)
                    value = myCondition.substr(op + 1, endValue + 2); // myCondition.substr(op + 2, endValue); -> if we don't want the ' around the text
                else
                {
                    output.success = false;
                    output.errorMessage = "Missing ' symbol at the end of your value from condition operator in your SELECT query";
                    return output;
                }
            }

            // Add condition to the list
            condition_str cond; // Create a new condition
            cond.attribute = att;
            cond.op = myCondition.substr(op, op + 1)[0];
            cond.value = value;
            cond.log = log_op;
            conditions->push_back(cond);
        }
        else
        {
            output.success = false;
            output.errorMessage = "Invalid value from condition operator in your SELECT query";
        }
    }
    else
    {
        output.success = false;
        output.errorMessage = "Missing condition operator in your SELECT query";
    }
    return output;
}

structure_str select_query(string query)
{
    structure_str output;
    string myQuery = query;

    size_t endSymbol = query.find(']'); // Stop the seraching at the first ']' symbol
    if (endSymbol != std::string::npos) // Check if the symbol exists
    {
        vector<condition_str> conditions;
        output = condition(query.substr(0, endSymbol), &conditions); // [condition] - Get the text before the ']' symbol, send the pointer of the list of conditions

        if (output.success) // Check if the condition has no errors
        {
            //Add relation
            string rel = relation(query.substr(endSymbol + 1, query.length()));
            if (rel != "")
                output = readFromCSV(rel);
            else
            {
                output.success = false;
                output.errorMessage = "Invalid relation query syntax";
            }
        }
    }
    else
    {
        output.success = false;
        output.errorMessage = "You have an error in your SELECT query syntax";
    }

    return output;
}

// [Relation] (Regex)
structure_str relation_regex(string rel, string type)
{
    structure_str output;
    if (rel.find('()[]') == std::string::npos)
        output = readFromCSV(rel);
    else // Recursive Query
        output = process_query_regex(rel);

    return output;
}

// SELECT Query (Regex) We don't have to validate anymore!!!
structure_str select_query_regex(string query)
{
    structure_str output, outputRel;

    // [condition]
    size_t condPos = query.find_first_of(']');
    string condListStr = query.substr(0, condPos);
    condListStr = condListStr.substr(7, condListStr.length());

    regex rx_attribute_extract("[,|]?([^ \\[\\]{}<>=!,|\\n]*)([{}<>=!])('[^']*'|[0-9]*)");
    bool first = true;
    vector<condition_str> conditions;
    for (sregex_iterator i = sregex_iterator(condListStr.begin(), condListStr.end(), rx_attribute_extract); i != sregex_iterator(); ++i)
    {
        string attStr = (*i).str();
        size_t pos = attStr.find_first_of("<>=!{}");
        if (first)
        {
            attStr.insert(0, ",");
            pos++;
            first = false;
        }
        // Add condition to the list
        condition_str cond; // Create a new condition
        cond.attribute = attStr.substr(1, pos - 1);
        cond.op = attStr[pos];
        cond.value = attStr.substr(pos + 1, attStr.length());
        if (cond.value[0] == '\'')
        {
            cond.value = cond.value.substr(1, cond.value.length());
            cond.value = cond.value.substr(0, cond.value.length() - 1);
        }
        cond.log = attStr[0];
        conditions.push_back(cond);
    }

    // (relation)
    string rel = (*sregex_iterator(query.begin(), query.end(), regex("\\([^ \\n]*\\)"))).str();
    rel = rel.substr(1, rel.length() - 2);
    output = outputRel = relation_regex(rel, "SELECT");

    // CSV
    for (int i = 0; i < conditions.size(); ++i)
    {
        if (conditions[i].attribute == "True")
            continue;
        else if (conditions[i].attribute == "False")
        {
            int p = conditions.size() - 1;
            while (p != -1)
            {
                if (conditions[p].attribute != "False")
                {
                    condition_str cond;
                    cond.attribute = conditions[p].attribute;
                    cond.op = '>';
                    cond.value = "0";

                    conditions.push_back(cond);
                    cond.op = '<';
                    conditions.push_back(cond);
                    p = -1;
                }
                else
                    p--;
            }
        }
        else if (conditions[i].log == ',')
            output = do_select_query(&output, conditions[i]);
        else
        {
            structure_str output1 = do_select_query(&outputRel, conditions[i]);
            output = do_union(&output, &output1);
        }
    }

    return output;
}

// PROJECT Query (Regex)
structure_str project_query_regex(string query)
{
    structure_str output;

    // [attribute]
    size_t attPos = query.find_first_of(']');
    string attListStr = query.substr(0, attPos);
    attListStr = attListStr.substr(8, attListStr.length());

    regex rx_attribute_extract("[^ \\[\\]<>=!,|\\n]+");
    bool first = true;
    vector<string> attributes;
    for (sregex_iterator i = sregex_iterator(attListStr.begin(), attListStr.end(), rx_attribute_extract); i != sregex_iterator(); ++i)
    {
        string attStr = (*i).str();
        attributes.push_back(attStr);
    }

    // (relation)
    string rel = (*sregex_iterator(query.begin(), query.end(), regex("\\([^ \\n]*\\)"))).str();
    rel = rel.substr(1, rel.length() - 2);
    output = relation_regex(rel, "PROJECT");

    output = do_project(&output, attributes);

    return output;
}

// UNION/CARTESIAN/JOIN/DIFFERENCE Query (Regex)
structure_str ucjd_query_regex(string query, string type)
{
    structure_str output1, output2, output;

    string getFirstRel = (*sregex_iterator(query.begin(), query.end(), regex("^" + type + "\\(([^()\\n]*|[^()\\n]*\\((?1)\\)[^()\\n]*)*\\)"))).str(); // ^UNION\(([^()\n]*|[^()\n]*\((?1)\)[^()\n]*)*\)
    string rel1 = getFirstRel.substr(1 + type.length(), getFirstRel.length()); // DELETE: UNION(
    rel1 = rel1.substr(0, rel1.length() - 1); // DELETE the last )

    // (relation_1)
    output1 = relation_regex(rel1, type + "(1)");

    // (relation_2)
    string rel2 = query.substr(getFirstRel.length() + 1, getFirstRel.length());
    rel2 = rel2.substr(0, rel2.length() - 1);
    output2 = relation_regex(rel2, type + "(2)");

    switch (hashit(type)) // Process Query
    {
        case UNION:
        {
            output = do_union(&output1, &output2);
            break;
        }
        case CARTESIAN:
        {
            output = do_cartesianProduc(&output1, &output2);
            break;
        }
        case DIFFERENCE:
        {
            output = do_minus(&output1, &output2);
            break;
        }
        default:
        {
            output.success = false;
            output.errorMessage = "OMG, that's a nasty bug ^_^";
        }   
    }

    return output;
}

// RENAME Query (Regex)
structure_str rename_query_regex(string query)
{
    structure_str output;

    // [new_attribute(attribute)]
    size_t attPos = query.find_first_of(']');
    string attListStr = query.substr(0, attPos);
    attListStr = attListStr.substr(7, attListStr.length());

    regex rx_attribute_extract("[^ \\[\\]<>=!,|\\n]+");
    bool first = true;
    map <string, string> mapAttributes;
    vector<string> attributes;
    for (sregex_iterator i = sregex_iterator(attListStr.begin(), attListStr.end(), rx_attribute_extract); i != sregex_iterator(); ++i)
    {
        string attStr2 = (*i).str();
        string newAttribute = attStr2.substr(0, attStr2.find_first_of('('));
        string attribute = attStr2.substr(newAttribute.length() + 1, query.length());
        attribute = attribute.substr(0, attribute.length() - 1);

        attributes.push_back(newAttribute);
        mapAttributes.insert(make_pair(newAttribute, attribute));
    }

    // (relation)
    string rel = query.substr(attPos + 1, query.length());
    rel = rel.substr(1, rel.length() - 2);
    output = relation_regex(rel, "RENAME");

    for (int i = 0; i < attributes.size(); ++i)
        output = do_renaming(&output, mapAttributes[attributes[i]], attributes[i]);

    return output;
}

structure_str project_cartesian_query_regex(string query)
{
    // [attribute]
    size_t attPos = query.find_first_of(']');
    string attListStr = query.substr(0, attPos);
    attListStr = attListStr.substr(8, attListStr.length());

    regex rx_attribute_extract("[^ \\[\\]<>=!,|\\n]+");
    bool first = true;
    vector<string> attributes;
    for (sregex_iterator i = sregex_iterator(attListStr.begin(), attListStr.end(), rx_attribute_extract); i != sregex_iterator(); ++i)
    {
        string attStr = (*i).str();
        attributes.push_back(attStr);
    }

    // (relation) -> CARTESIAN
    string rel = (*sregex_iterator(query.begin(), query.end(), regex("\\([^ \\n]*\\)"))).str();
    rel = rel.substr(1, rel.length() - 2); // CARTESIAN

    // Process CARTESIAN
    structure_str outputCART1, outputCART2;
    string getFirstRel = (*sregex_iterator(rel.begin(), rel.end(), regex("^CARTESIAN\\(([^()\\n]*|[^()\\n]*\\((?1)\\)[^()\\n]*)*\\)"))).str(); // ^CARTESIAN\(([^()\n]*|[^()\n]*\((?1)\)[^()\n]*)*\)
    string rel1 = getFirstRel.substr(1 + 9, getFirstRel.length()); // DELETE: CARTESIAN(
    rel1 = rel1.substr(0, rel1.length() - 1); // DELETE the last )

    // (relation_1)
    outputCART1 = relation_regex(rel1, "CARTESIAN(1)");

    // (relation_2)
    string rel2 = rel.substr(getFirstRel.length() + 1, getFirstRel.length());
    rel2 = rel2.substr(0, rel2.length() - 1);
    outputCART2 = relation_regex(rel2, "CARTESIAN(2)");

    // If we have common attributes we change the 2nd attrbitue's name
    structure_str tmp = outputCART2;
    for (auto& categorie : outputCART2.attributes) {
        if (existeAndIndex_attribute(outputCART1.attributes, categorie) != -1) {
            tmp = do_renaming(&outputCART2, categorie, categorie + "2");
        }
    }
    outputCART2 = tmp;

    // Check which attributes corresponds to whom
    vector<string> attrRel1, attrRel2;
    // Attributes of CARTESIAN(1)
    for (auto& categorie : outputCART1.attributes) {
        if (existeAndIndex_attribute(attributes, categorie) != -1) {
            attrRel1.push_back(categorie);
        }
    }
    // Attributes of CARTESIAN(2)
    for (auto& categorie : outputCART2.attributes) {
        if (existeAndIndex_attribute(attributes, categorie) != -1) {
            attrRel2.push_back(categorie);
        }
    }
    
    // DO PROJECT -> THEN CARTESIAN
    structure_str output1, output2, output;
    output1 = do_project(&outputCART1, attrRel1);
    output2 = do_project(&outputCART2, attrRel2);

    if(optParallel)
        output = do_cartesianProductParrallele(&output1, &output2); // OPT CARTESIAN
    else
        output = do_cartesianProduc(&output1, &output2); // non-OPT CARTESIAN
    
    return output;
}

structure_str jpr_query_regex(string query)
{
    structure_str output;

    // [new_attribute(attribute)]
    size_t attPos = query.find_first_of(']');
    string attListStr = query.substr(0, attPos);
    attListStr = attListStr.substr(3, attListStr.length());

    regex rx_attribute_extract("[^ \\[\\]<>=!,|\\n]+");
    bool first = true;
    map <string, string> mapAttributes;
    vector<string> newAttributes;
    vector<string> oldAttributes;
    for (sregex_iterator i = sregex_iterator(attListStr.begin(), attListStr.end(), rx_attribute_extract); i != sregex_iterator(); ++i)
    {
        string attStr2 = (*i).str();
        string newAttribute = attStr2.substr(0, attStr2.find_first_of('('));
        string attribute = attStr2.substr(newAttribute.length() + 1, query.length());
        attribute = attribute.substr(0, attribute.length() - 1);

        oldAttributes.push_back(attribute);
        newAttributes.push_back(newAttribute);
    }

    // [condition]
    string subQuery = query.substr(attPos + 2, query.length());
    size_t condPos = subQuery.find_first_of(']');
    string condListStr = subQuery.substr(0, condPos);

    regex rx_attribute_extract2("[,|]?([^ \\[\\]{}<>=!,|\\n]*)([{}<>=!])('[^']*'|[0-9]*)");
    first = true;
    vector<condition_str> conditions;
    for (sregex_iterator i = sregex_iterator(condListStr.begin(), condListStr.end(), rx_attribute_extract2); i != sregex_iterator(); ++i)
    {
        string attStr = (*i).str();
        size_t pos = attStr.find_first_of("<>=!{}");
        if (first)
        {
            attStr.insert(0, ",");
            pos++;
            first = false;
        }
        // Add condition to the list
        condition_str cond; // Create a new condition
        cond.attribute = attStr.substr(1, pos - 1);
        cond.op = attStr[pos];
        cond.value = attStr.substr(pos + 1, attStr.length());
        if (cond.value[0] == '\'')
        {
            cond.value = cond.value.substr(1, cond.value.length());
            cond.value = cond.value.substr(0, cond.value.length() - 1);
        }
        cond.log = attStr[0];
        conditions.push_back(cond);
    }

    // (CARTESIAN1)(CARTESIAN2)
    subQuery = query.substr(attPos + condPos + 3, query.length());
    subQuery = subQuery.substr(0, subQuery.length());

    string getFirstRel = (*sregex_iterator(subQuery.begin(), subQuery.end(), regex("\\(([^()\\n]*|[^()\\n]*\\((?1)\\)[^()\\n]*)*\\)"))).str();
    string rel1 = getFirstRel.substr(1 , getFirstRel.length()); // DELETE: (
    rel1 = rel1.substr(0, rel1.length() - 1); // DELETE the last )

    structure_str output1, output2;
    // (relation_1)
    output1 = relation_regex(rel1, "CARTESIAN(1)");

    // (relation_2)
    string rel2 = subQuery.substr(getFirstRel.length() + 1, getFirstRel.length());
    rel2 = rel2.substr(0, rel2.length() - 1);
    output2 = relation_regex(rel2, "CARTESIAN(2)");

    if(optParallel)
        output = joinProjectRename_parra(&output1, &output2, oldAttributes, oldAttributes, newAttributes, conditions); // OPT JPR
    else
        output = joinProjectRename(&output1, &output2, oldAttributes, oldAttributes, newAttributes, conditions); // non-OPT JPR

    return output;
}

// Which type of query is? (Regex)
structure_str process_query_regex(string query)
{
    structure_str output;
    regex rgx_SELECT("^SELECT\\[([^ \\[\\]{}<>=!,|\\n]+[{}<>=!]('[^']+'|[0-9]+)(?(?=\\,|\\|)((\\,|\\|)(?1))))\\]\\(([^()\\n]+|[^()\\n]*\\((?5)\\)[^()\\n]*)+\\)"); // ^SELECT\[([^ \[\]{}<>=!,|\n]+[{}<>=!]('[^']+'|[0-9]+)(?(?=\,|\|)((\,|\|)(?1))))\]\(([^()\n]+|[^()\n]*\((?5)\)[^()\n]*)+\)
    regex rgx_PROJECTION("^PROJECT\\[([^ \\[\\]<>=!,|\\n]+(?(?=\\,)((\\,)(?1))))\\]\\(([^()\\n]+|[^()\\n]*\\((?4)\\)[^()\\n]*)+\\)"); // ^PROJECT\[([^ \[\]<>=!,|\n]+(?(?=\,)((\,)(?1))))\]\(([^()\n]+|[^()\n]*\((?4)\)[^()\n]*)+\)
    regex rgx_UNION("^UNION\\(.+\\)\\(.+\\)");
    regex rgx_CARTESIAN("^CARTESIAN\\(.+\\)\\(.+\\)");
    regex rgx_DIFFERENCE("^DIFFERENCE\\(.+\\)\\(.+\\)");
    regex rgx_RENAME("^RENAME\\[([^ \\[\\]<>=!,|\\n]+\\([^ \\[\\]<>=!,|\\n]+\\)(?(?=\\,)((\\,)(?1))))\\]\\(([^()\\n]+|[^()\\n]*\\((?4)\\)[^()\\n]*)+\\)"); // ^RENAME\[([^ \[\]<>=!,|\n]+\([^ \[\]<>=!,|\n]+\)(?(?=\,)((\,)(?1))))\]\(([^()\n]+|[^()\n]*\((?4)\)[^()\n]*)+\)
    regex rgx_PROJECT_CARTESIAN("^PROJECT\\[([^ \\[\\]<>=!,|\\n]+(?(?=\\,)((\\,)(?1))))\\]\\(CARTESIAN\\(([^()\\n]+|[^()\\n]*\\((?4)\\)[^()\\n]*)+\\)\\(([^()\\n]+|[^()\\n]*\\((?5)\\)[^()\\n]*)+\\)\\)"); // ^PROJECT\[([^ \[\]<>=!,|\n]+(?(?=\,)((\,)(?1))))\]\(CARTESIAN\(([^()\n]+|[^()\n]*\((?4)\)[^()\n]*)+\)\(([^()\n]+|[^()\n]*\((?5)\)[^()\n]*)+\)\)
    regex rgx_JPR("^JPR\\[([^ \\[\\]<>=!,|\\n]+\\([^ \\[\\]<>=!,|\\n]+\\)(?(?=\\,)((\\,)(?1))))\\]\\[([^ \\[\\]{}<>=!,|\\n]+[{}<>=!]('[^']+'|[0-9]+)(?(?=\\,|\\|)((\\,|\\|)(?4))))\\]\\(.+\\)\\(.+\\)"); // ^JPR\[([^ \[\]<>=!,|\n]+\([^ \[\]<>=!,|\n]+\)(?(?=\,)((\,)(?1))))\]\[([^ \[\]{}<>=!,|\n]+[{}<>=!]('[^']+'|[0-9]+)(?(?=\,|\|)((\,|\|)(?4))))\]\(.+\)\(.+\)

    if (regex_match(query, rgx_SELECT)) // SELECT
        output = select_query_regex(query);
    else if (optPC && regex_match(query, rgx_PROJECT_CARTESIAN))  // PROJECT(CARTESIAN)
        output = project_cartesian_query_regex(query);
    else if (regex_match(query, rgx_PROJECTION)) // PROJECT
        output = project_query_regex(query);
    else if (regex_match(query, rgx_UNION)) // UNION
        output = ucjd_query_regex(query, "UNION");
    else if (regex_match(query, rgx_CARTESIAN)) // CARTESIAN
        output = ucjd_query_regex(query, "CARTESIAN");
    else if (regex_match(query, rgx_DIFFERENCE)) // DIFFERENCE
        output = ucjd_query_regex(query, "DIFFERENCE");
    else if (regex_match(query, rgx_JPR)) // JPR
        output = jpr_query_regex(query);
    else if (regex_match(query, rgx_RENAME)) // RENAME
        output = rename_query_regex(query);
    else
        output.success = false;

    return output;
}


// Which type of query is?
structure_str process_query(string query)
{
    structure_str output;
    string myQuery = query;

    size_t pos = myQuery.find('['); // Stop the seraching at the first '[' symbol
    if (pos != string::npos) // Check if the symbol exists
    {
        switch (hashit(myQuery.substr(0, pos))) // Get the text before the '[' symbol
        {
        case SELECT:
        {
            output = select_query(myQuery.substr(pos + 1, myQuery.length())); // Pass the text after the '[' symbol
            break;
        }
        default:
            output.success = false;
        }
    }
    else
        output.success = false;

    return output;
}

// JSON
string readJSONfile(string file)
{
    // Load JSON file
    ifstream jsonFile(file);
    json jsonQuery;
    jsonFile >> jsonQuery;

    string queryRA;
    queryRA += queryJSON(jsonQuery);

    return queryRA;
}

// Query JSON
string queryJSON(json jsonQuery)
{
    string operation;

    if (jsonQuery["operation"].empty())
        return "ERROR";

    operation = jsonQuery["operation"];
    if (operation == "load")
    {
        string queryRA = jsonQuery["filename"];
        queryRA = queryRA.substr(0, queryRA.length() - 4); // Remove .csv
        return queryRA;
    }
    else
    {
        if (jsonQuery["args"].empty())
            return "ERROR";
        jsonQuery = jsonQuery["args"];

        if (operation == "selection") // SELECT JSON is actually PROJECT
            return "PROJECT[" + attributeJSON(jsonQuery["attributes"]) + "](" + queryJSON(jsonQuery["object"]) + ")"; // [attribute] - ATTS (relation) - R
        else if (operation == "projection") // PROJECT JSON is actually SELECT
            return "SELECT[" + conditionJSON(jsonQuery["condition"], 0) + "](" + queryJSON(jsonQuery["object"]) + ")"; // [condition] - COND (relation) - R
        else if (operation == "product")
            return "CARTESIAN(" + queryJSON(jsonQuery["object1"]) + ")(" + queryJSON(jsonQuery["object2"]) + ")"; // (relation) - R (relation) - R
        else if (operation == "minus")
            return "DIFFERENCE(" + queryJSON(jsonQuery["object1"]) + ")(" + queryJSON(jsonQuery["object2"]) + ")"; // (relation) - R (relation) - R
        else if (operation == "union")
            return "UNION(" + queryJSON(jsonQuery["object1"]) + ")(" + queryJSON(jsonQuery["object2"]) + ")"; // (relation) - R (relation) - R
        else if (operation == "renaming")
        {
            string queryRA = "RENAME[";
            unsigned int count = 0;
            for (auto& elem : jsonQuery["old attributes"].items())
                queryRA += string(jsonQuery["new attributes"][count++]) + "(" + string(elem.value()) + "),";
            queryRA = queryRA.substr(0, queryRA.length() - 1);
            queryRA += "](" + queryJSON(jsonQuery["object"]) + ")";

            return queryRA;
        }
        else if (operation == "rspr")
        {
            string queryRA = "RENAME[";
            unsigned int count = 0;
            for (auto& elem : jsonQuery["old attributes"].items())
                queryRA += string(jsonQuery["new attributes"][count++]) + "(" + string(elem.value()) + "),";
            queryRA = queryRA.substr(0, queryRA.length() - 1);
            queryRA += "](";
            
            queryRA += "SELECT[" + conditionJSON(jsonQuery["condition"], 0) + "](PROJECT[" + attributeJSON(jsonQuery["old attributes"]) + "](" + string(jsonQuery["filename"]);
            queryRA = queryRA.substr(0, queryRA.length() - 4); // Remove .csv
            queryRA += ")))";
            
            return queryRA;
        }
        else if (operation == "jpr")
        {
            string queryRA = "JPR[";
            unsigned int count = 0;
            for (auto& elem : jsonQuery["old attributes"].items())
                queryRA += string(jsonQuery["new attributes"][count++]) + "(" + string(elem.value()) + "),";
            queryRA = queryRA.substr(0, queryRA.length() - 1);
            queryRA += "][";
            queryRA += conditionJSON(jsonQuery["condition"], 0) + "](" + queryJSON(jsonQuery["object1"]) + ")(" + queryJSON(jsonQuery["object2"]) + ")";

            return queryRA;
        }
    }

    return "ERROR";
}

// [condition] JSON
string conditionJSON(json jsonQuery, bool negativeLogic = 0)
{
    string queryRA;
    if (jsonQuery.empty())
        return "ERROR";

    if (jsonQuery == "False" || jsonQuery == "True")
        queryRA += string(jsonQuery) + "=1";
    else
    if (!jsonQuery["comparator"].empty())
    {
        queryRA += string(jsonQuery["attribute1"]);
        if(!negativeLogic)
            queryRA += string(jsonQuery["comparator"]);
        else
        {
            if (jsonQuery["comparator"] == ">")
                queryRA += "{";
            else if (jsonQuery["comparator"] == "<")
                queryRA += "}";
            else if (jsonQuery["comparator"] == "}")
                queryRA += "<";
            else if (jsonQuery["comparator"] == "{")
                queryRA += ">";
            else if (jsonQuery["comparator"] == "=")
                queryRA += "!";
            else if (jsonQuery["comparator"] == "!")
                queryRA += "=";
        }
        queryRA += (is_number(jsonQuery["attribute2"]) ? string(jsonQuery["attribute2"]) : "'" + string(jsonQuery["attribute2"]) + "'");
    }
    else if (!jsonQuery["logical"].empty())
    {
        if ((jsonQuery["logical"] == "and" && !negativeLogic) || (jsonQuery["logical"] == "or" && negativeLogic))
        {
            queryRA += conditionJSON(jsonQuery["condition1"], negativeLogic);
            queryRA += ",";
            queryRA += conditionJSON(jsonQuery["condition2"], negativeLogic);
        }
        else if ((jsonQuery["logical"] == "or" && !negativeLogic) || (jsonQuery["logical"] == "and" && negativeLogic))
        {
            queryRA += conditionJSON(jsonQuery["condition1"], negativeLogic);
            queryRA += "|";
            queryRA += conditionJSON(jsonQuery["condition2"], negativeLogic);
        }
        else if (jsonQuery["logical"] == "not")
            queryRA += conditionJSON(jsonQuery["condition"], !negativeLogic);
    }

    return queryRA;
}

// [attribute] JSON
string attributeJSON(json jsonQuery)
{
    string queryRA;
    if (jsonQuery.empty())
        return "";

    for (auto& elem : jsonQuery.items())
        queryRA += string(elem.value()) + ",";
    queryRA = queryRA.substr(0, queryRA.length() - 1); // Remove the last ,

    return queryRA;
}
