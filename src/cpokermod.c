// Copyright 2013 Allen Boyd Cunningham

// This file is part of pypoker-tools.

//     pypoker_tools is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//     pypoker_tools is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.

//     You should have received a copy of the GNU General Public License
//     along with pypoker_tools.  If not, see <http://www.gnu.org/licenses/>.


#include "poker_heavy.h"
#include "Python.h"


static int convert_cards(PyObject *pycard_list, uint32_t *cards, int ncards){
    int i;
    PyObject * pycard;

    if ( !PyList_Check(pycard_list) ){
        PyErr_SetString(PyExc_TypeError, "Hand and board must be lists");
        return FAIL;
    }

    if ( PyList_GET_SIZE(pycard_list) != ncards ){
        PyErr_Format(PyExc_TypeError, "got %i cards, expected %i",
            (int) PyList_GET_SIZE(pycard_list), ncards);
        return FAIL;
    }

    for (i = 0; i < ncards; i++){
        pycard = PyList_GET_ITEM(pycard_list, i);
        if (!PyInt_Check(pycard)){
            PyErr_SetString(PyExc_TypeError, "cards must be ints");
            return FAIL;
        }
        cards[i] = PyInt_AsLong( pycard );
    }

    return 1;
}


#define SET_LIST_BY_TYPE(typefunc, list, array, len) \
    PyObject * item; \
    for (i = 0; i < len; i++){ \
        item = typefunc(array[i]); \
        PyList_SetItem(list, i, item); \
    }

static PyObject *buildListFromArray( void *array, int len, char dtype){

    PyObject * plist;
    int i;

    plist = PyList_New(len);

    switch (dtype){
        case 'i': {int *iptr = (int*) array;
                  SET_LIST_BY_TYPE(PyInt_FromLong, plist, iptr, len)
        break;}
        case 'd': {double *dptr = (double*) array;
                  SET_LIST_BY_TYPE(PyFloat_FromDouble, plist, dptr, len)
        break;}
        default:
        printf("i'll only support int or double, sorry\n");
        exit(EXIT_FAILURE);
    }
    return plist;
}


static PyObject *cpoker_handvalue(PyObject *self, PyObject *args){
    PyObject *pyhand;
    uint32_t chand[7];
    if ( ! PyArg_ParseTuple(args, "O", &pyhand ) )
        return NULL;

    if (convert_cards(pyhand, chand, 7) == FAIL){
        return NULL;
    }
    return (PyObject*) PyLong_FromLongLong(handvalue(chand));
}


const char holdem2p_doc[] =
"holdem2p(hand1, hand2, board) -> integer\n\n"
"Return the winner according to the following:\n"
"0 -> hand1 wins\n"
"1 -> hand2 wins\n"
"2 -> tie\n";

static PyObject *cpoker_holdem2p(PyObject *self, PyObject *args){
    PyObject *pyh1, *pyh2, *pyboard;
    uint32_t ch1[2], ch2[2], cboard[5];
    if ( ! PyArg_ParseTuple(args, "OOO", &pyh1, &pyh2, &pyboard ) )
        return NULL;

    if (convert_cards(pyboard, cboard, 5) == FAIL){
        return NULL;
    }
    if (convert_cards(pyh1, ch1, 2) == FAIL){
        return NULL;
    }
    if (convert_cards(pyh2, ch2, 2) == FAIL){
        return NULL;
    }
    return (PyObject*) PyInt_FromLong(holdem2p(ch1, ch2, cboard));
}


const char multi_holdem_doc[] =
"multi_holdem(hands, board) -> list\n\n"
"Return the indices of all hands tied for the win.\n"
"hands -> list of hands\n"
"board -> five card board\n";


static PyObject *cpoker_multi_holdem(PyObject *self, PyObject *args){
    PyObject *pyhands, *pyboard;
    uint32_t chands[MAX_HANDS][2], cboard[5];
    int winners[MAX_HANDS] = {-1,-1,-1,-1,-1};
    int nhands, nwinners, i;

    if ( ! PyArg_ParseTuple(args, "OO", &pyhands, &pyboard ) )
        return NULL;

    if ( (nhands = PyList_Size(pyhands)) == FAIL ){
        PyErr_SetString(PyExc_TypeError, "multi_holdem requires a list of hands");
        return NULL;
    }

    for (i = 0; i < nhands; i++){
        if (convert_cards(PyList_GetItem(pyhands, i), chands[i], 2) == FAIL){
            return NULL;
        }
    }

    if (convert_cards(pyboard, cboard, 5) == FAIL){
        return NULL;
    }
    nwinners = multi_holdem(chands, nhands, cboard, winners);
    return (PyObject*) buildListFromArray(winners, nwinners, 'i');
}



const char rivervalue_doc[] =
"rivervalue(hand, board, [optimistic]) -> float\n\n"
"Return the ev ( (wins + 0.5ties) / total ) of hand\n"
"vs all 990 opposing hand combinations;\n\n"
"Optionally, supplying True for optimistic returns\n"
"(wins + ties) / total.\n";

static PyObject * cpoker_rivervalue ( PyObject * self, PyObject * args )
{
    PyObject *pyhand, *pyboard;
    uint32_t hand[2], board[5];
    int optimistic = 0;
    double tie_bonus;
    struct rivervalue value;
    static const double nmatches = 990;

    if (!PyArg_ParseTuple(args, "OO|i", &pyhand, &pyboard, &optimistic))
        return NULL;

    if (convert_cards(pyhand, hand, 2) == FAIL){
        return NULL;
    }

    if (convert_cards(pyboard, board, 5) == FAIL){
        return NULL;
    }

    value = rivervalue(hand, board);
    if ( value.wins == FAIL ){
        PyErr_SetString(PyExc_ValueError, "duplicate cards");
        return NULL;
    }
    tie_bonus = (optimistic) ? value.ties : (value.ties / 2.0);
    return (PyObject *) PyFloat_FromDouble( (value.wins + tie_bonus) / nmatches );
}

const char riverties_doc[] =
"riverties(hand, board) -> tuple\n\n"
"Return the tuple <number of wins>, <number of ties>\n"
"vs all 990 opposing hand combinations\n";

static PyObject * cpoker_riverties ( PyObject * self, PyObject * args )
{
    PyObject *pyhand, *pyboard;
    uint32_t hand[2], board[5];
    struct rivervalue value;

    if (!PyArg_ParseTuple(args, "OO", &pyhand, &pyboard))
        return NULL;

    if (convert_cards(pyhand, hand, 2) == FAIL){
        return NULL;
    }

    if (convert_cards(pyboard, board, 5) == FAIL){
        return NULL;
    }

    value = rivervalue(hand, board);
    if ( value.wins == FAIL ){
        PyErr_SetString(PyExc_ValueError, "duplicate cards");
        return NULL;
    }
    return (PyObject *) Py_BuildValue( "ii", value.wins, value.ties );
}

const char full_enumeration_doc[] =
"full_enumeration(hands) -> list\n\n"
"Return a list of evs for each respective hand.\n\n"
"This is accomplished by counting wins and ties for\n"
"each hand on every possible board combination.\n"
"Ties are rewarded 1.0/ntied the score of a win.\n"
"This is optimized for 2 players, ie. 3 players is around 3xslower.\n";


static PyObject * cpoker_full_enumeration ( PyObject * self, PyObject * args )
{
    PyObject *list;
    uint32_t hands[MAX_HANDS][2];
    double results[MAX_HANDS];
    int i, nhands;

    if (!PyArg_ParseTuple(args, "O", &list))
        return NULL;

    if ( (nhands = PyList_Size(list)) < 1 ){ // this also happens if 'list' is not a list
        PyErr_SetString(PyExc_TypeError, "full_enumeration requires a list of hands");
        return NULL;
    }
    if (nhands == 1){
        return (PyObject *) Py_BuildValue("[d]", 1.0);
    }

    for (i = 0; i < nhands; i++){
        if (convert_cards(PyList_GetItem(list, i), hands[i], 2) == FAIL){
            return NULL;
        }
    }

    if (nhands == 2){
        if ( (results[0] = enum2p(hands[0], hands[1])) == FAIL ){
            PyErr_SetString(PyExc_ValueError, "duplicate cards");
            return NULL;
        }
        results[1] = 1.0 - results[0];
    }

    else if ( full_enumeration(hands, results, nhands) == FAIL ){
        PyErr_SetString(PyExc_ValueError, "duplicate cards");
        return NULL;
    }
    return (PyObject *) buildListFromArray( results, nhands, 'd');
}


int GET_INDEX(uint32_t c1, uint32_t c2){
    //hash sceme for holdem hand
    if ( c1 > c2 ){
        //swap algo
        c1 = c1 ^ c2;
        c2 = c1 ^ c2;
        c1 = c1 ^ c2;
    }
    return c1 * 52 - c1 * (c1 + 1) / 2 + c2 - c1 - 1;
}

#define MAX_PREFLOP_GROUPS 32


//pyList is a list of values in appropriate order
//the order should be the same as itertools.combinations
int setHandDictWithList(PyObject * pyList, dictEntry handDict[]){
    int i, j, index = 0;
    dictEntry *dict = &handDict[0];

    int max = 0;

    if (PyList_Size(pyList) != NUM_STARTING_HANDS){
        PyErr_SetString(PyExc_ValueError, "list must contain 1326 entries (one for each starting hand)");
        return FAIL;
    }

    for (i = 0; i < 52; i++){
        for (j = i + 1; j < 52; j++, dict++, index++){
            dict->hand[0] = i;
            dict->hand[1] = j;
            dict->value = PyInt_AsLong(PyList_GetItem(pyList, index));
            if (dict->value > max)
                max = dict->value;
        }
    }

    #ifdef DEBUG
    for (i = 0; i < NUM_STARTING_HANDS; i ++){
        if ( handDict[i].value != PyInt_AsLong(PyList_GetItem(pyList, i)) ){
            printf("test failed: cval %i != pyval %li\n", handDict[i].value,
                PyInt_AsLong(PyList_GetItem(pyList, i)));
            return FAIL;
        }

    }
    #endif
    return max;
}


//set handDict using values from pyDict
//pyDict must include all two card hand combinations
//with values <= MAX_PREFLOP_GROUPS
int setHandDictWithDict(PyObject *pyDict, dictEntry handDict[]){
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    int c1, c2, index;
    int max = 0;

    if (PyDict_Size(pyDict) != NUM_STARTING_HANDS){
        PyErr_SetString(PyExc_ValueError, "dictionary must contain 1326 entries (one for each starting hand)");
        return FAIL;
    }

    while (PyDict_Next(pyDict, &pos, &key, &value)){

        if ( !PyTuple_Check(key) ){
            PyErr_SetString(PyExc_ValueError, "dictionary keys must be card tuples");
            return FAIL;
        }
        if ( !PyArg_ParseTuple(key, "ii", &c1, &c2) ){
            return FAIL;
        }
        //if the pyDict is the right length and no GET_INDEX
        //is > NUM_STARTING_HANDS we will necessarily fill in handDict
        if ( (index = GET_INDEX(c1, c2)) >= NUM_STARTING_HANDS ){
            PyErr_SetString(PyExc_ValueError, "dictionary keys must be tuples of unmatching cards (0-51)");
            return FAIL;
        }
        handDict[index].hand[0] = c1;
        handDict[index].hand[1] = c2;
        if ( (handDict[index].value = PyInt_AsLong(value)) == FAIL){
            PyErr_SetString(PyExc_ValueError, "dictionary values must be integers");
            return FAIL;
        }
        if (handDict[index].value > max)
            max = handDict[index].value;

    }
    return max;
}


int set_dict ( PyObject *phand_values, dictEntry handDict[] )
{

    int max = FAIL;

    if ( PyList_Check(phand_values) ){
        max = setHandDictWithList(phand_values, handDict);
    }
    else if ( PyDict_Check(phand_values) ){
        max = setHandDictWithDict(phand_values, handDict);
    }
    else{
        PyErr_SetString(PyExc_ValueError, "hand_values must be a list or dict");
    }

    if (max > MAX_PREFLOP_GROUPS){
        PyErr_Format(PyExc_ValueError,
            "preflop value too high.  got %i, needed <= %i",
            max, MAX_PREFLOP_GROUPS);
        return FAIL;
    }
    return max;
}

const char river_distribution_doc[] =
"river_distribution(hand, board, [hand_values])\n\n"
"Return a histogram showing how your hand does against\n"
"different groups of preflop hands.\n\n"
"The preflop groups are set by hand_values, which is saved\n"
"between calls thus needs only be supplied once unless you\n"
"change it.\n\n"
"The histogram bars give you 2 points for each win and 1 point\n"
"for each tie for all hands in that group\n"
"hand, board -> lists of cards, 2 and 5\n"
"hand_values -> either a dictionary or list mapping all preflop\n"
"    hands to a hand group (such as the Sklansky hand ranks)\n"
"    Dictionary keys must be tuples of cards with the smallest\n"
"    of the two cards first.\n"
"    List items must be the values sorted by keys.\n"
"    Values must be contiguous integers starting from 0.\n";

static PyObject * cpoker_river_distribution(PyObject *self, PyObject *args){

    static dictEntry handDict[NUM_STARTING_HANDS];
    static int maxDictValue = FAIL;
    //save a reference to the phand_values object
    //to check if the same object is continuously used
    static PyObject *oldvalues = NULL;


    PyObject *pyhand, *pyboard;
    PyObject *phand_values = NULL;
    uint32_t hand[2], board[5], i;
    int chart[MAX_PREFLOP_GROUPS];

    if (!PyArg_ParseTuple(args, "OO|O", &pyhand, &pyboard, &phand_values))
        return NULL;

    //make sure not to waste time resetting handDict if the same
    //phand_values is used multiple times
    if ( phand_values && (phand_values != oldvalues) ){
        if ( (maxDictValue = set_dict(phand_values, handDict)) == FAIL ){
            return NULL;
        }
        Py_XDECREF(oldvalues);
        Py_INCREF(phand_values);
        oldvalues = phand_values;
    }

    if (maxDictValue == FAIL){
        PyErr_SetString(PyExc_ValueError, "no proper hand_values are set");
        return NULL;
    }

    if (convert_cards(pyhand, hand, 2) == FAIL){
        return NULL;
    }

    if (convert_cards(pyboard, board, 5) == FAIL){
        return NULL;
    }

    for ( i = 0; i <= maxDictValue; i++)
        chart[i] = 0;

    if ( river_distribution(hand, board, chart, handDict) == FAIL ){
        PyErr_SetString(PyExc_ValueError, "duplicate cards");
        return NULL;
    }

    return (PyObject *) buildListFromArray( chart, maxDictValue + 1, 'i');
}


void printdeck(void){
    void printcard(int);
    int r;
    for (r = 0; r < 52; r++){
        if (r % 4 == 0)
            printf("\n");
        printcard(r);
    }
    printf("\n");
    }


static PyMethodDef cpokerMethods[] = {
    { "handvalue", cpoker_handvalue, METH_VARARGS },
    { "holdem2p", cpoker_holdem2p, METH_VARARGS, holdem2p_doc },
    { "multi_holdem", cpoker_multi_holdem, METH_VARARGS, multi_holdem_doc},
    { "rivervalue", cpoker_rivervalue, METH_VARARGS, rivervalue_doc },
    { "riverties", cpoker_riverties, METH_VARARGS, riverties_doc },
    { "full_enumeration", cpoker_full_enumeration, METH_VARARGS, full_enumeration_doc },
    { "river_distribution", cpoker_river_distribution, METH_VARARGS, river_distribution_doc },
    { NULL, NULL }
};


PyMODINIT_FUNC initcpoker (void)
{
    extern uint16_t Rank_Table[7825760];
    extern uint16_t Flush_Table[8129];
    extern uint16_t Straight_Table[8129];
    populate_tables(Rank_Table, Flush_Table, Straight_Table);

    (void) Py_InitModule("cpoker", cpokerMethods);
}
