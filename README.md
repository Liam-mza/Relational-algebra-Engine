# Relational-algebra-Engine
Relational algebra Engine that interprets and executes queries on a database!

# samples folder
**JSON_List_Examples.json** -> List of JSON queries that works with the current implementation.

**query.json** -> Store here the JSON query which you want to use in the project. Use `query> load` command to run the json file.

**Terminal_List_Examples.txt** -> List of Terminal queries that work with the current implementation.

# Project Libraries
**\#include <boost/regex.hpp>** -> Used for complex regex (with recursion)
[Tutorial how to setup Boost C++](https://www.youtube.com/watch?v=5afpq2TkOHc&t=248s)

The reason we chose this library is that the regex implementation from the C++ standard library **\#include <regex>** does not support recursion, which would be needed to match nested structures. In this case we use [Boost.Regex](https://www.boost.org/doc/libs/1_75_0/libs/regex/doc/html/index.html)

**\#include "include/json.hpp"** -> Used to store the JSON file into a _nlohmann::json_ variable, this way it is easier to access the data from the file (e.g. _jsonQuery["operation"]["selection"]["attributes"]_)

JSON format is parsed into Terminal format and then it is passed to the REGEX validation. Some functions are implemented at the JSON level and not at the Terminal language level. For instance logical **not** is loaded from a JSON file and then the corresponding Terminal language is built based on the query's condition.

# RA Engine
We have implemented all expressions: selection, project, cartesian product, renaming, minus and union. For complex queries we use recursion and then we output the result in a nice table on Terminal.

## Terminal Commands
**>query test** -> Start a predefined test queries with and without optimization

**>query load** -> Load query from JSON file

**>query enable OPT** -> Enable cartesian optimization (PART 2)

**>query disable OPT** -> Disable cartesian optimization

**>query enable PARA** -> Enable parallel functions (PART 3)

**>query disable PARA** -> Disable parallel functions

**>query exit** -> Exit the Terminal

## Terminal Language
**[condition]: attribute_1#value_1$attribute_2#value_2** including _False=1_ and _True=1_
**(attribute): attribute_1,attribute_2**

**1. Selection**: SELECT[condition] (relation)
```
SELECT[email='tellus@aceleifend.co.uk'](employes)
SELECT[ide<5,dpt>3,email='tellus@aceleifend.co.uk'](employes)
SELECT[ide<5](SELECT[dpt>3](employes))
SELECT[ide<5,dpt>3](employes)
SELECT[ide<5,ide!3](employes)
SELECT[ide{5](employes)
SELECT[ide}95](employes)
SELECT[ide<1|dpt>10](employes)
SELECT[ide<1|dpt>15|email='tellus@aceleifend.co.uk'](employes)
SELECT[ide<5,dpt>3,ide>0,ide<0](employes)
SELECT[False=1,ide<5,dpt>3](employes)
SELECT[ide<5,dpt>3,True=1](employes)
```

**2. Projection**: PROJECT[attribute] (relation)
```
PROJECT[ide,dpt,nom](employes)
PROJECT[ide,email](employes)
PROJECT[ide,dpt](SELECT[ide{5](employes))
PROJECT[dpt,idp,nom](CARTESIAN(SELECT[ide<5](employes))(SELECT[idp<5](projets)))
PROJECT[dpt,nom,idp](CARTESIAN(SELECT[ide>5](employes))(membres))
```

**3. Cartesian**: CARTESIAN(relation_1)(relation_2)
```
CARTESIAN(employes)(departements)
CARTESIAN(SELECT[ide<3](employes))(SELECT[idp<5](projets))
```

**4. Renaiming**: RENAME[new_attribute_name(attribute)] (relation)
```
RENAME[new_ide(ide),new_dpt(dpt)](SELECT[ide<3|dpt>15](employes))
```

**5. Difference**: DIFFERENCE(relation_1)(relation_2)
```
DIFFERENCE(SELECT[ide<3](employes))(SELECT[dpt>9](employes))
```

**6. Union**: UNION(relation_1)(relation_2)
```
UNION(employes)(employes)
UNION(SELECT[dpt>15](employes))(SELECT[ide<30](employes))
```

**7. JPR**: JPR[new_attribute_name(attribute)] [condition] (relation)(relation)
```
JPR[new_ide(ide),new_nom(nom),new_titre(titre)][ide<5,dpt>3](employes)(projets)
```

# Json parser
To load the JSON file **query.json**, type `query> load` command in Terminal to process the data and show the result on Terminal.


# Regex
A naïve implementation of query's condition and relation mapping was implemented based on splitting the query in multiple string parts and storing each attribute, operator and value into variables. We decided to use Regex in order to validate the full query, to reduce time, to write fewer lines of code and to access easier and faster the attributes and conditions from the string query.

An example of a REGEX which was used for PROJECT(CARTESIAN) optimization, is the following:
`^PROJECT\[([^ \[\]<>=!,|\n]+(?(?=\,)((\,)(?1))))\]\(CARTESIAN\(([^()\n]+|[^()\n]*\((?4)\)[^()\n]*)+\)\(([^()\n]+|[^()\n]*\((?5)\)[^()\n]*)+\)\)`

```
^PROJECT -> query starts with PROJECT
\[([^ \[\]<>=!,|\n]+(?(?=\,)((\,)(?1))))\] -> [1] it contain one or more attributes 
\(CARTESIAN -> query follows a _CARTESIAN(_ query inside the PROJECT relation
\(([^()\n]+|[^()\n]*\((?4)\)[^()\n]*)+\) -> (2) it contains a relation or another query
\(([^()\n]+|[^()\n]*\((?5)\)[^()\n]*)+\) -> (3) it contains a relation or another query
\) -> end the CARTESIAN query with _)_
```

# Parallel computation
To be able to compute queries faster, we decided to implement a form parallelism by using multi-threading for the operations that are compatible with it. These operations are therefore Cartesian products and JoinProjectRename.To enable parallelism you only need to type `enable PARA` and to disable it type  `disable PARA`
**WARNING**: if your computer does not have enough cores (4) or if the cpu does not support hyperthreading this might not be efficient. 

**PLEASE NOTE** that if your computer does not have enough cores (4) or if the cpu does not support hyperthreading or if your query is not calling Cartesian product, Select or JoinProjectRename you won’t see any differences in the times because the optimization we’ve done won’t be used. 

# Example
here is an example of query with the terminal output:  
<img src="drawing.jpg" alt="drawing" width="200"/>
