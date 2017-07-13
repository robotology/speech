SPEECH USING START
=====================

These modules use the [**START Natural Language Question Answering System**](http://start.csail.mit.edu/index.php) in order to retrieve information about the speech query.

**START**, the world's first `Web-based question answering system`, has been on-line and continuously operating since December, 1993. It has been developed by `Boris Katz` and his associates of the InfoLab Group at the `MIT Computer Science and Artificial Intelligence Laboratory`. Unlike information retrieval systems (e.g., search engines), START aims to supply users with "just the right information," instead of merely providing a list of hits. Currently, the system can answer millions of English questions about places (e.g., cities, countries, lakes, coordinates, weather, maps, demographics, political and economic systems), movies (e.g., titles, actors, directors), people (e.g., birth dates, biographies), dictionary definitions, and much, much more.

The repository contains the following modules:

1. **ask:** This module is the question based interface to the **START** answering system. It takes as an input a question and outputs the reply from **START**
2. **input:** This module interfaces with the **START** server to retrieve semantic information about a query.
3. **inspect:** This module serves of a helper to the **input** module described above interpreting the its output for ease of use.
