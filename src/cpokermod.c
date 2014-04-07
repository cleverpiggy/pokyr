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
        Py_INCREF(item); \
        PyList_SetItem(list, i, item); \
    }

static PyObject *buildListFromArray( void *buffer, int len, char dtype){

    PyObject * plist;
    int i;

    plist = PyList_New(len);
    Py_INCREF(plist);

    switch (dtype){
        case 'i': {long *iptr = (long*) buffer;
                  SET_LIST_BY_TYPE(PyInt_FromLong, plist, iptr, len)
        break;}
        case 'd': {double *dptr = (double*) buffer;
                  SET_LIST_BY_TYPE(PyFloat_FromDouble, plist, dptr, len)
        break;}
        default:
        printf("i'll only support int (long) or double, sorry\n");
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


const char holdem_doc[] =
"holdem(hand1, hand2, board)\n\n"
"Return the winner according to the following:\n"
"0 -> hand1 wins\n"
"1 -> hand2 wins\n"
"2 -> tie\n";

static PyObject *cpoker_holdem(PyObject *self, PyObject *args){
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
    return (PyObject*) PyInt_FromLong(holdem(ch1, ch2, cboard));
}


const char rivervalue_doc[] =
"rivervalue(hand, board, [optimistic])\n\n"
"Return the ev ( (wins + 0.5ties) / total ) of hand\n"
" vs all 990 opposing hand combinations;\n\n"
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
"riverties(hand, board)\n\n"
"Return the tuple <number of wins>, <number of ties>\n"
" vs all 990 opposing hand combinations\n";

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



static PyObject * cpoker_preflop_match ( PyObject * self, PyObject * args )
{
    PyObject *pyh1, *pyh2;
    uint32_t h1[2], h2[2];
    double result;

    if (!PyArg_ParseTuple(args, "OO", &pyh1, &pyh2))
        return NULL;

    if (convert_cards(pyh1, h1, 2) == FAIL){
        return NULL;
    }

    if (convert_cards(pyh2, h2, 2) == FAIL){
        return NULL;
    }

    result = preflop_match(h1, h2);
    if ( result == -1 ){
        PyErr_SetString(PyExc_ValueError, "duplicate cards");
        return NULL;
    }
    return (PyObject *) PyFloat_FromDouble( result );
}


//for example
#define GET_INDEX(c1, c2) c1 * 52 - c1 * (c1 + 1) / 2 + c2 - c1 - 1

#define MAX_PREFLOP_GROUPS 32

dictEntry handDict[NUM_STARTING_HANDS] = {(dictEntry) {.value = -1}};
int maxDictValue;

//pyList is a list of values in appropriate order
//the order should be the same as itertools.combinations
void setHandDict(PyObject * pyList){
    int i, j, index = 0;
    dictEntry *dict = &handDict[0];

    for (i = 0; i < 52; i++){
        for (j = i + 1; j < 52; j++, dict++, index++){
            dict->hand[0] = i;
            dict->hand[1] = j;
            dict->value = PyInt_AsLong(PyList_GetItem(pyList, index));
        }
    }
}


static PyObject * cpoker_set_dict ( PyObject * self, PyObject * args )
{
    PyObject *plist;
    int i;

    if (!PyArg_ParseTuple(args, "O", &plist))
        return NULL;

    if ( !PyList_Check(plist) || !(PyList_Size(plist) == NUM_STARTING_HANDS) ){
        PyErr_SetString(PyExc_TypeError, "Do better with the list of values");
        return NULL;
    }
    setHandDict(plist);
    maxDictValue = 0;

    for (i = 0; i < NUM_STARTING_HANDS; i ++){
        if ( handDict[i].value != PyInt_AsLong(PyList_GetItem(plist, i)) ){
            printf("test failed: cval %i != pyval %li\n", handDict[i].value,
                PyInt_AsLong(PyList_GetItem(plist, i)));
            return NULL;
        }
        if (handDict[i].value > maxDictValue)
            maxDictValue = handDict[i].value;
    }
    if (maxDictValue > MAX_PREFLOP_GROUPS){
        PyErr_Format(PyExc_ValueError,
            "preflop value too high.  got %i, needed <= %i",
            maxDictValue, MAX_PREFLOP_GROUPS);
        return NULL;
    }
    printf("passed with max value %i\n", maxDictValue);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * cpoker_river_distribution(PyObject *self, PyObject *args){
    PyObject *pyhand, *pyboard;
    uint32_t hand[2], board[5], i;
    long chart[MAX_PREFLOP_GROUPS];

    if (!PyArg_ParseTuple(args, "OO", &pyhand, &pyboard))
        return NULL;

    if (handDict[0].value == -1){
        PyErr_SetString(PyExc_TypeError, "no handDict!");
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
    { "holdem", cpoker_holdem, METH_VARARGS, holdem_doc },
    { "rivervalue", cpoker_rivervalue, METH_VARARGS, rivervalue_doc },
    { "riverties", cpoker_riverties, METH_VARARGS, riverties_doc },
    { "preflop_match", cpoker_preflop_match, METH_VARARGS },
    { "set_dict", cpoker_set_dict, METH_VARARGS },
    { "river_distribution", cpoker_river_distribution, METH_VARARGS },
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
