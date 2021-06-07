import argparse
import os.path
import signal
import subprocess
import copy
from sets import Set

DEFAULT_EXECUTABLE_PATH = './sheet'
DEFAULT_LOG_PATH = './log'
DEFAULT_TIMEOUT = 3

ARGS = 0
INVALID = 1
MODIFICATION = 2
SELECT = 4
PROCESSING = 5

EMPTY_STRING = '$$EMPTY-STRING$$'
STRINGS = ['help', 'a', 'very-long-string-to-test-very-long-items', EMPTY_STRING]
INVALIDS = ['0', '-5', 'a', 'STRING']
CODE_SUCCESS = 0
DEFAULT_DELIMITER = ' '
SELECT_CONSTRAIN = 10

TABLE1 = (':', [
    ['table', 'A', 'B'],
    ['1', 'A1', 'B1'],
    ['2', 'A2', 'B2'] ],
    ['tab', 'B', 'le', 'X'])

TABLE2 = (',;', [
    ['numbers', '0.69', '0.47', '-999'],
    ['-7968', '168.245', '7', '0'],
    ['6.7', '0000', '-3.974', '0.00001'],
    ['0.5555', '-51', '1', '4.47413685'] ],
    ['num', '-', '5', 'nothing'])

TABLEBAD = (':', [
    ['col1', 'col2', 'col3'],
    ['col1', 'col3']
    ])

logger = None

class TimeoutException(Exception):
    pass

def GetInvalidCombos():
    result = []
    for i1 in (INVALIDS + ['1']):
        for i2 in (INVALIDS + ['1']):
            if (i1 != '1' or i2 != '1'):
                result.append(i1 + ' ' + i2)
    return result

def GetTooLongString():
    result = ''
    for i in range(110):
        result += '.'
    return result

def AlarmHandle(signum, frame):
    raise TimeoutException()

def Cols(table):
    result = []
    for i in range(len(table[1][0])):
        result.append(str(i+1))
    return result

def Rows(table):
    result = []
    for i in range(len(table[1])):
        result.append(str(i+1))
    return result

def GenerateColStringCombinations(table, strings=STRINGS):
    result = []
    for i in Cols(table):
        for j in strings:
            result.append(i + ' ' + j)
    return result

def GenerateTwoCols(table):
    result = []
    for i in Cols(table):
        for j in Cols(table):
            result.append(i + ' ' + j)
    return result

def GenerateColRange(table):
    result = []
    for i in Cols(table):
        for j in Cols(table):
            if i <= j:
                result.append(i + ' ' + j)
    return result

def GenerateRowRange(table):
    result = []
    for i in Rows(table):
        for j in Rows(table):
            if i <= j:
                result.append(i + ' ' + j)
    return result

def GenerateSelectCommands(table):
    result = []
    for item in GenerateRowRange(table):
        result.append('rows ' + item)
    for item in Rows(table):
        result.append('rows ' + item + ' -')
    result.append('rows - -')

    for item in GenerateColStringCombinations(table, table[2]):
        result.append('beginswith ' + item)
    for item in GenerateColStringCombinations(table, table[2]):
        result.append('contains ' + item)
    return result

def GenerateModificationCommands(table):
    result = []
    for cmd in ['irow', 'arow', 'drow', 'drows', 'icol', 'acol', 'dcol', 'dcols']:
        if cmd in ['arow', 'acol']:
            result.append(cmd)
        elif cmd in ['irow', 'drow', 'icol', 'dcol']:
            for item in Cols(table):
                result.append(cmd + ' ' + item)
        elif cmd == 'drows':
            for item in GenerateRowRange(table):
                result.append(cmd + ' ' + item)
        else:
            for item in GenerateColRange(table):
                result.append(cmd + ' ' + item)
    return result

def CreateTests(disabled):

    def CreateTest(table, commands, categories, name=None, delimiter=None, isError=False):
        for category in categories:
            if category in disabled:
                return []
        test = {}
        test['name'] = name if name is not None else ' '.join(commands)
        test['table'] = table
        test['commands'] = commands
        test['delimiter'] = delimiter
        test['isError'] = isError
        return test

    tests = []

    tests.append(CreateTest(TABLE1, [], [ARGS], name='bad delimiter argument', delimiter='', isError=True))
    tests.append(CreateTest(TABLE1, ['invalid-command'], [ARGS], name='invalid command', isError=True))
    tests.append(CreateTest(TABLE1, ['cset 1 HELP', 'cset 2 HELP'], [ARGS, PROCESSING], name='multiple processing commands', isError=True))
    tests.append(CreateTest(TABLE1, ['rows 1 1', 'irow 1'], [ARGS, MODIFICATION], name='select and modification commands', isError=True))
    tests.append(CreateTest(TABLE1, ['acol', 'contains 2 b'], [ARGS, MODIFICATION, SELECT], name='modification and select commands', isError=True))
    tests.append(CreateTest(TABLE1, ['drow 2', 'tolower 1'], [ARGS, MODIFICATION, PROCESSING], name='modification and processing commands', isError=True))
    tests.append(CreateTest(TABLE1, ['copy 3 2', 'dcols 1 2'], [ARGS, MODIFICATION, PROCESSING], name='processing and modification commands', isError=True))
    tests.append(CreateTest(TABLE1, ['startswith 1 ' + GetTooLongString()], [ARGS, SELECT], name='startswith too large item', isError=True))
    tests.append(CreateTest(TABLE1, ['contains 1 ' + GetTooLongString()], [ARGS, SELECT], name='contains too large item', isError=True))
    tests.append(CreateTest(TABLE1, ['cset 1 ' + GetTooLongString()], [ARGS, PROCESSING], name='cset too large item', isError=True))
    tests.append(CreateTest(TABLEBAD, [], [ARGS], name='bad columns', isError=True))

    for invalid in INVALIDS:
        tests.append(CreateTest(TABLE1, ['irow ' + invalid], [ARGS, MODIFICATION], isError=True))
    for invalid in INVALIDS:
        tests.append(CreateTest(TABLE1, ['drow ' + invalid], [ARGS, MODIFICATION], isError=True))
    for invalid in GetInvalidCombos():
        tests.append(CreateTest(TABLE1, ['drows ' + invalid], [ARGS, MODIFICATION], isError=True))
    tests.append(CreateTest(TABLE1, ['drows 3 1'], [ARGS, MODIFICATION], isError=True))
    for invalid in INVALIDS:
        tests.append(CreateTest(TABLE1, ['icol ' + invalid], [ARGS, MODIFICATION], isError=True))
    for invalid in INVALIDS:
        tests.append(CreateTest(TABLE1, ['dcol ' + invalid], [ARGS, MODIFICATION], isError=True))
    for invalid in GetInvalidCombos():
        tests.append(CreateTest(TABLE1, ['dcols ' + invalid], [ARGS, MODIFICATION], isError=True))
    tests.append(CreateTest(TABLE1, ['dcols 3 1'], [ARGS, MODIFICATION], isError=True))

    for invalid in INVALIDS:
        tests.append(CreateTest(TABLE1, ['cset ' + invalid + ' str'], [ARGS, PROCESSING], isError=True))
    for invalid in INVALIDS:
        tests.append(CreateTest(TABLE1, ['tolower ' + invalid], [ARGS, PROCESSING], isError=True))
    for invalid in INVALIDS:
        tests.append(CreateTest(TABLE1, ['toupper ' + invalid], [ARGS, PROCESSING], isError=True))
    for invalid in INVALIDS:
        tests.append(CreateTest(TABLE1, ['round ' + invalid], [ARGS, PROCESSING], isError=True))
    for invalid in INVALIDS:
        tests.append(CreateTest(TABLE1, ['int ' + invalid], [ARGS, PROCESSING], isError=True))
    for invalid in GetInvalidCombos():
        tests.append(CreateTest(TABLE1, ['copy ' + invalid], [ARGS, PROCESSING], isError=True))
    for invalid in GetInvalidCombos():
        tests.append(CreateTest(TABLE1, ['swap ' + invalid], [ARGS, PROCESSING], isError=True))
    for invalid in GetInvalidCombos():
        tests.append(CreateTest(TABLE1, ['move ' + invalid], [ARGS, PROCESSING], isError=True))

    for invalid in GetInvalidCombos():
        tests.append(CreateTest(TABLE1, ['rows ' + invalid], [ARGS, SELECT], isError=True))
    for invalid in INVALIDS:
        tests.append(CreateTest(TABLE1, ['beginswith ' + invalid + ' str'], [ARGS, SELECT], isError=True))
    for invalid in INVALIDS:
        tests.append(CreateTest(TABLE1, ['contains ' + invalid + ' str'], [ARGS, SELECT], isError=True))

    tests.append(CreateTest(TABLE1, ['round 2'], [INVALID, PROCESSING], isError=True))
    tests.append(CreateTest(TABLE1, ['int 3'], [INVALID, PROCESSING], isError=True))
    tests.append(CreateTest(TABLE1, ['round 1'], [INVALID, PROCESSING], isError=True))
    tests.append(CreateTest(TABLE1, ['int 1'], [INVALID, PROCESSING], isError=True))
    tests.append(CreateTest(TABLE1, ['cset 8 str'], [INVALID, PROCESSING]))
    tests.append(CreateTest(TABLE1, ['tolower 5'], [INVALID, PROCESSING]))
    tests.append(CreateTest(TABLE1, ['toupper 4'], [INVALID, PROCESSING]))
    tests.append(CreateTest(TABLE1, ['round 6'], [INVALID, PROCESSING]))
    tests.append(CreateTest(TABLE1, ['int 7'], [INVALID, PROCESSING]))
    tests.append(CreateTest(TABLE1, ['copy 2 10'], [INVALID, PROCESSING]))
    tests.append(CreateTest(TABLE1, ['copy 9 1'], [INVALID, PROCESSING]))
    tests.append(CreateTest(TABLE1, ['copy 4 8'], [INVALID, PROCESSING]))
    tests.append(CreateTest(TABLE1, ['swap 3 9'], [INVALID, PROCESSING]))
    tests.append(CreateTest(TABLE1, ['swap 8 2'], [INVALID, PROCESSING]))
    tests.append(CreateTest(TABLE1, ['swap 12 13'], [INVALID, PROCESSING]))
    tests.append(CreateTest(TABLE1, ['move 11 1'], [INVALID, PROCESSING]))
    tests.append(CreateTest(TABLE1, ['move 3 7'], [INVALID, PROCESSING]))
    tests.append(CreateTest(TABLE1, ['move 19 27'], [INVALID, PROCESSING]))

    tests.append(CreateTest(TABLE1, ['rows 5 10', 'cset 1 str'], [INVALID, SELECT, PROCESSING]))
    tests.append(CreateTest(TABLE1, ['beginswith 99 A', 'cset 1 str'], [INVALID, SELECT, PROCESSING]))
    tests.append(CreateTest(TABLE1, ['contains 4 B', 'cset 1 str'], [INVALID, SELECT, PROCESSING]))

    tests.append(CreateTest(TABLE1, ['irow 9'], [INVALID, MODIFICATION]))
    tests.append(CreateTest(TABLE1, ['drow 5'], [INVALID, MODIFICATION]))
    tests.append(CreateTest(TABLE1, ['drows 12 13'], [INVALID, MODIFICATION]))
    tests.append(CreateTest(TABLE1, ['icol 6'], [INVALID, MODIFICATION]))
    tests.append(CreateTest(TABLE1, ['dcol 8'], [INVALID, MODIFICATION]))
    tests.append(CreateTest(TABLE1, ['dcols 7 7'], [INVALID, MODIFICATION]))

    for item in GenerateColStringCombinations(TABLE1):
        tests.append(CreateTest(TABLE1, ['cset ' + item], [PROCESSING]))
    for item in Cols(TABLE1):
        tests.append(CreateTest(TABLE1, ['tolower ' + item], [PROCESSING]))
    for item in Cols(TABLE1):
        tests.append(CreateTest(TABLE1, ['toupper ' + item], [PROCESSING]))
    for item in GenerateTwoCols(TABLE1):
        tests.append(CreateTest(TABLE1, ['copy ' + item], [PROCESSING]))
    for item in GenerateTwoCols(TABLE1):
        tests.append(CreateTest(TABLE1, ['swap ' + item], [PROCESSING]))
    for item in GenerateTwoCols(TABLE1):
        tests.append(CreateTest(TABLE1, ['move ' + item], [PROCESSING]))

    for item in GenerateColStringCombinations(TABLE2):
        tests.append(CreateTest(TABLE2, ['cset ' + item], [PROCESSING]))
    for item in Cols(TABLE2):
        tests.append(CreateTest(TABLE2, ['tolower ' + item], [PROCESSING]))
    for item in Cols(TABLE2):
        tests.append(CreateTest(TABLE2, ['toupper ' + item], [PROCESSING]))
    for item in [2, 3, 4]:
        tests.append(CreateTest(TABLE2, ['round ' + str(item)], [PROCESSING]))
    for item in [2, 3, 4]:
        tests.append(CreateTest(TABLE2, ['int ' + str(item)], [PROCESSING]))
    for item in GenerateTwoCols(TABLE2):
        tests.append(CreateTest(TABLE2, ['copy ' + item], [PROCESSING]))
    for item in GenerateTwoCols(TABLE2):
        tests.append(CreateTest(TABLE2, ['swap ' + item], [PROCESSING]))
    for item in GenerateTwoCols(TABLE2):
        tests.append(CreateTest(TABLE2, ['move ' + item], [PROCESSING]))

    index = 0
    for s in GenerateSelectCommands(TABLE1):
        for item in GenerateColStringCombinations(TABLE1):
            if index%SELECT_CONSTRAIN == 0:
                tests.append(CreateTest(TABLE1, [s, 'cset ' + item], [PROCESSING, SELECT]))
            index = index + 1
        for item in Cols(TABLE1):
            if index%SELECT_CONSTRAIN == 0:
                tests.append(CreateTest(TABLE1, [s, 'tolower ' + item], [PROCESSING, SELECT]))
            index = index + 1
        for item in Cols(TABLE1):
            if index%SELECT_CONSTRAIN == 0:
                tests.append(CreateTest(TABLE1, [s, 'toupper ' + item], [PROCESSING, SELECT]))
            index = index + 1
        for item in GenerateTwoCols(TABLE1):
            if index%SELECT_CONSTRAIN == 0:
                tests.append(CreateTest(TABLE1, [s, 'copy ' + item], [PROCESSING, SELECT]))
            index = index + 1
        for item in GenerateTwoCols(TABLE1):
            if index%SELECT_CONSTRAIN == 0:
                tests.append(CreateTest(TABLE1, [s, 'swap ' + item], [PROCESSING, SELECT]))
            index = index + 1
        for item in GenerateTwoCols(TABLE1):
            if index%SELECT_CONSTRAIN == 0:
                tests.append(CreateTest(TABLE1, [s, 'move ' + item], [PROCESSING, SELECT]))
            index = index + 1

    for s in GenerateSelectCommands(TABLE2):
        for item in GenerateColStringCombinations(TABLE2):
            if index%SELECT_CONSTRAIN == 0:
                tests.append(CreateTest(TABLE2, [s, 'cset ' + item], [PROCESSING, SELECT]))
            index = index + 1
        for item in Cols(TABLE2):
            if index%SELECT_CONSTRAIN == 0:
                tests.append(CreateTest(TABLE2, [s, 'tolower ' + item], [PROCESSING, SELECT]))
            index = index + 1
        for item in Cols(TABLE2):
            if index%SELECT_CONSTRAIN == 0:
                tests.append(CreateTest(TABLE2, [s, 'toupper ' + item], [PROCESSING, SELECT]))
            index = index + 1
        for item in GenerateTwoCols(TABLE2):
            if index%SELECT_CONSTRAIN == 0:
                tests.append(CreateTest(TABLE2, [s, 'copy ' + item], [PROCESSING, SELECT]))
            index = index + 1
        for item in GenerateTwoCols(TABLE2):
            if index%SELECT_CONSTRAIN == 0:
                tests.append(CreateTest(TABLE2, [s, 'swap ' + item], [PROCESSING, SELECT]))
            index = index + 1
        for item in GenerateTwoCols(TABLE2):
            if index%SELECT_CONSTRAIN == 0:
                tests.append(CreateTest(TABLE2, [s, 'move ' + item], [PROCESSING, SELECT]))
            index = index + 1

    for item in GenerateModificationCommands(TABLE1):
        tests.append(CreateTest(TABLE1, [item], [MODIFICATION]))

    for item in GenerateModificationCommands(TABLE2):
        tests.append(CreateTest(TABLE2, [item], [MODIFICATION]))
    

    result = []
    for test in tests:
        if test != []:
            result.append(test)
    return result

def ProcessCommands(table, commands):

    def GetMapping(mapping, index):
        if index > len(mapping):
            return -1
        else:
            return mapping[index-1]

    def DeleteRow(row, newTable, mapping):
        index = GetMapping(mapping, row)
        result = []
        for i in range(len(newTable)):
            if i != index:
                result.append(newTable[i])
        tmp = []
        for item in mapping:
            if item >= index:
                tmp.append(item - 1)
            else:
                tmp.append(item)
        return tmp, result

    def DeleteCol(col, newTable, mapping):
        index = GetMapping(mapping, col)
        new = []
        for row in newTable:
            result = []
            i = 0
            for col in row:
                if i != index:
                    result.append(row[i])
                i = i + 1
            new.append(result)
        newTable = new
        tmp = []
        for item in mapping:
            if item >= index:
                tmp.append(item - 1)
            else:
                tmp.append(item)
        return tmp, newTable

    newTable = copy.deepcopy(table[1])
    currentColumns = len(newTable[0])
    mappingRows = []
    for i in range(len(newTable)):
        mappingRows.append(i)
    mappingCols = []
    for i in range(len(newTable[0])):
        mappingCols.append(i)
    select = []
    for i in range(len(table[1])):
        select.append(i)

    for command in commands:
        cmd = command.split()
        for item in cmd:
            if item == EMPTY_STRING:
                item = ''

        if cmd[0] == 'rows':
            begin = len(newTable) if (cmd[1] == '-') else int(cmd[1])
            end = len(newTable) if (cmd[2] == '-') else int(cmd[2])
            select = []
            for i in range(begin, end+1):
                if i <= len(newTable):
                    select.append(i-1)

        elif cmd[0] == 'beginswith':
            select = []
            col = int(cmd[1])-1
            if col < len(newTable[0]):
                for i in range(len(newTable)):
                    if newTable[i][col].startswith(cmd[2]):
                        select.append(i)

        elif cmd[0] == 'contains':
            select = []
            col = int(cmd[1])-1
            if col < len(newTable[0]):
                for i in range(len(newTable)):
                    if cmd[2] in newTable[i][col]:
                        select.append(i)

        elif cmd[0] == 'cset':
            col = int(cmd[1])-1
            if col < len(newTable[0]):
                for i in range(len(newTable)):
                    if i in select:
                        newTable[i][col] = cmd[2]

        elif cmd[0] == 'tolower':
            col = int(cmd[1])-1
            if col < len(newTable[0]):
                for i in range(len(newTable)):
                    if i in select:
                        newTable[i][col] = newTable[i][col].lower()

        elif cmd[0] == 'toupper':
            col = int(cmd[1])-1
            if col < len(newTable[0]):
                for i in range(len(newTable)):
                    if i in select:
                        newTable[i][col] = newTable[i][col].upper()

        elif cmd[0] == 'round':
            col = int(cmd[1])-1
            if col < len(newTable[0]):
                for i in range(len(newTable)):
                    if i in select:
                        newTable[i][col] = str(int(round(float(newTable[i][col]))))

        elif cmd[0] == 'int':
            col = int(cmd[1])-1
            if col < len(newTable[0]):
                for i in range(len(newTable)):
                    if i in select:
                        newTable[i][col] = str(int(float(newTable[i][col])))

        elif cmd[0] == 'copy':
            src = int(cmd[1])-1
            dst = int(cmd[2])-1
            if src < len(newTable[0]) and dst < len(newTable[0]):
                for i in range(len(newTable)):
                    if i in select:
                        newTable[i][dst] = newTable[i][src]

        elif cmd[0] == 'swap':
            src = int(cmd[1])-1
            dst = int(cmd[2])-1
            if src < len(newTable[0]) and dst < len(newTable[0]):
                for i in range(len(newTable)):
                    if i in select:
                        tmp = newTable[i][src]
                        newTable[i][src] = newTable[i][dst]
                        newTable[i][dst] = tmp

        elif cmd[0] == 'move':
            src = int(cmd[1])-1
            dst = int(cmd[2])-1
            if src < len(newTable[0]) and dst < len(newTable[0]) and src != dst:
                for i in range(len(newTable)):
                    if i in select:
                        newRow = []
                        for j in range(len(newTable[i])):
                            if j == src:
                                pass
                            elif j == dst:
                                newRow.append(newTable[i][src])
                                newRow.append(newTable[i][j])
                            else:
                                newRow.append(newTable[i][j])
                        newTable[i] = newRow

        elif cmd[0] == 'irow':
            index = GetMapping(mappingRows, int(cmd[1]))
            if index != -1:
                result = []
                for i in range(len(newTable)):
                    if i == index:
                        newRow = []
                        for j in range(currentColumns):
                            newRow.append('')
                        result.append(newRow)
                    result.append(newTable[i])
                newTable = result
                tmp = []
                for item in mappingRows:
                    if item >= index:
                        tmp.append(item + 1)
                    else:
                        tmp.append(item)
                mappingRows = tmp

        elif cmd[0] == 'arow':
            newRow = []
            for i in range(currentColumns):
                newRow.append('')
            newTable.append(newRow)

        elif cmd[0] == 'drow':
            mappingRows, newTable = DeleteRow(int(cmd[1]), newTable, mappingRows)

        elif cmd[0] == 'drows':
            for i in range(int(cmd[1]), int(cmd[2])+1):
                mappingRows, newTable = DeleteRow(i, newTable, mappingRows)

        elif cmd[0] == 'icol':
            currentColumns = currentColumns + 1
            index = GetMapping(mappingCols, int(cmd[1]))
            if index != -1:
                new = []
                for row in newTable:
                    result = []
                    i = 0
                    for col in row:
                        if i == index:
                            result.append('')
                        result.append(row[i])
                        i = i + 1
                    new.append(result)
                newTable = new
                tmp = []
                for item in mappingCols:
                    if item >= index:
                        tmp.append(item + 1)
                    else:
                        tmp.append(item)
                mappingCols = tmp

        elif cmd[0] == 'acol':
            currentColumns = currentColumns + 1
            for row in newTable:
                row.append('')

        elif cmd[0] == 'dcol':
            currentColumns = currentColumns - 1
            mappingCols, newTable = DeleteCol(int(cmd[1]), newTable, mappingCols)

        elif cmd[0] == 'dcols':
            for i in range(int(cmd[1]), int(cmd[2])+1):
                currentColumns = currentColumns -- 1
                mappingCols, newTable = DeleteCol(i, newTable, mappingCols)

        else:
            raise Exception('Unknown command \'' + cmd[0] + '\'')

    return (table[0], newTable)

# code, output, timeout
def RunExe(executable, timeout, args, table):
    try:
        cmd = [executable] + args
        signal.alarm(timeout)
        process = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        capture_out, capture_err = process.communicate(input=table)
        signal.alarm(0)
        if process.returncode == CODE_SUCCESS:
            return CODE_SUCCESS, capture_out, False
        else:
            return process.returncode, capture_err, False
    except TimeoutException:
        return None, None, True

def PrintPass(name):
    print('PASS : ' + name)

def PrintFail(name):
    print('FAIL : ' + name)

def LogUnexpectedError(name, inTable, args, output):
    global logger
    PrintFail(name)

    if logger is not None:
        logger.write('TEST: ' + name + '\nFAIL\n\n')
        logger.write('INPUT TABLE:\n' + inTable + '\n\n')
        logger.write('ARGUMENTS: ' + ' '.join(args) + '\n\n')
        logger.write('EXPECTED RESULT: ERROR\n\n')
        logger.write('OUTPUT:\n' + output + '\n\n')
        logger.write('--------------------\n\n')

def LogMatch(logSuccess, name, inTable, args, outTable):
    global logger

    if logSuccess:
        PrintPass(name)
        if logger is not None:
            logger.write('TEST: ' + name + '\nPASS\n\n')
            logger.write('INPUT TABLE:\n' + inTable + '\n\n')
            logger.write('ARGUMENTS: ' + ' '.join(args) + '\n\n')
            logger.write('RESULT:\n' + outTable + '\n\n')
            logger.write('--------------------\n\n')

def LogError(logSuccess, name, inTable, args, code, error):
    global logger

    if logSuccess:
        PrintPass(name)
        if logger is not None:
            logger.write('TEST: ' + name + '\nPASS\n\n')
            logger.write('INPUT TABLE:\n' + inTable + '\n\n')
            logger.write('ARGUMENTS: ' + ' '.join(args) + '\n\n')
            logger.write('ERROR CODE: ' + str(code) + '\n\n')
            logger.write('ERROR OUTPUT: ' + error + '\n\n')
            logger.write('--------------------\n\n')

def LogDontMatch(name, inTable, args, expTable, outTable):
    global logger
    PrintFail(name)

    if logger is not None:
        logger.write('TEST: ' + name + '\nFAIL\n\n')
        logger.write('INPUT TABLE:\n' + inTable + '\n\n')
        logger.write('ARGUMENTS: ' + ' '.join(args) + '\n\n')
        logger.write('EXPECTED RESULT:\n' + expTable + '\n\n')
        logger.write('ACTUAL RESULT:\n' + outTable + '\n\n')
        logger.write('--------------------\n\n')

def LogTimeouted(name, inTable, args, expected):
    global logger
    PrintFail(name)

    if logger is not None:
        logger.write('TEST: ' + name + '\nFAIL\n\n')
        logger.write('INPUT TABLE:\n' + inTable + '\n\n')
        logger.write('ARGUMENTS: ' + ' '.join(args) + '\n\n')
        logger.write('EXPECTED RESULT:\n' + expected + '\n\n')
        logger.write('ACTUAL RESULT: TIMEOUT\n\n')
        logger.write('--------------------\n\n')

def PrintTable(table, singleDelimiter=False):
    result = ""
    delimiters = DEFAULT_DELIMITER if table[0] is None else table[0]
    delimiter = 0
    for row in table[1]:
        for col in row:
            result += col + delimiters[delimiter]
            if not singleDelimiter:
                delimiter = delimiter + 1
                if delimiter >= len(delimiters):
                    delimiter = 0
        if row == []:
            result += '\n'
        else:
            result = result[:-1] + '\n'
    return result

def RunTest(name, executable, timeout, table, commands, logSuccess, delimiter, isError):
    if delimiter is None:
        delimiter = table[0]
    if delimiter is None:
        args = []
    elif delimiter == '':
        args = ['-d']
    else:
        args = ['-d', delimiter]
    for cmd in commands:
        args+=cmd.split()
    for item in args:
        if item == EMPTY_STRING:
            item = ''

    code, output, timeouted = RunExe(executable, timeout, args, PrintTable(table));

    if not isError:
        expected = ProcessCommands(table, commands)

    if timeouted:
        if isError:
            expected = "ERROR"
        else:
            expected = PrintTable(expected, True)
        LogTimeouted(name, PrintTable(table), args, expected)
        return False
    elif code == CODE_SUCCESS:
        if isError:
            LogUnexpectedError(name, PrintTable(table), args, output)
            return False
        elif output == PrintTable(expected, True):
            LogMatch(logSuccess, name, PrintTable(table), args, output)
            return True
        else:
            LogDontMatch(name, PrintTable(table), args, PrintTable(expected, True), output)
            return False
    else:
        if isError:
            LogError(logSuccess, name, PrintTable(table), args, code, output)
            return True
        else:
            LogDontMatch(name, PrintTable(table), args, PrintTable(expected, True), output)
            return False

def ParseArgs():
    # Define parser
    parser = argparse.ArgumentParser(description="run tests for IZP1 project")

   # Define all arguments
    parser.add_argument('--executable', '-e', default=DEFAULT_EXECUTABLE_PATH, help='path to the sheet executable. default: ' + DEFAULT_EXECUTABLE_PATH)
    parser.add_argument('--log-file', '-l', default=DEFAULT_LOG_PATH, help='path to the test log file (to be created). default: ' + DEFAULT_LOG_PATH)
    parser.add_argument('--log-success', '-s', default=False, action='store_true', help='additionaly prints successfull tests to the log (otherwise only failed tests are logged)')
    parser.add_argument('--timeout', '-t', default=DEFAULT_TIMEOUT, type=int, help='Maximum number of seconds that one test is allowed to run. default: ' + str(DEFAULT_TIMEOUT))
    parser.add_argument('--disable-args-errors', default=False, action='store_true', help='disables tests that checks incorrect command line arguments')
    parser.add_argument('--disable-invalid-commands', default=False, action='store_true', help='disables tests that checks invalid application of commands')
    parser.add_argument('--disable-modification', default=False, action='store_true', help='disables tests that checks usage of commands for table modification')
    parser.add_argument('--disable-select', default=False, action='store_true', help='disables tests that checks usage of commands for row selection')
    parser.add_argument('--disable-processing', default=False, action='store_true', help='disables tests that checks usage of commands for data processing')

    # Parse arguments from command line
    args = parser.parse_args()

    # Postprocessing
    if not os.path.isfile(args.executable):
        raise Exception('Executable \'' + args.executable + '\' is not a valid file')
    if os.path.isfile(args.log_file):
        print('WARNING: removing old log file \'' + args.log_file + '\'')
        os.remove(args.log_file)
    if os.path.isdir(args.log_file):
        raise Exception('There is a directory with the same name as specified log file \'' + args.log_file + '\'')
    if args.timeout <= 0:
        raise Exception('Value of timeout must be greater then zero, but it is \'' + str(args.timeout) + '\'')
    disabled = []
    if args.disable_args_errors:
        disabled.append(ARGS)
    if args.disable_invalid_commands:
        disabled.append(INVALID)
    if args.disable_modification:
        disabled.append(MODIFICATION)
    if args.disable_select:
        disabled.append(SELECT)
    if args.disable_processing:
        disabled.append(PROCESSING)

    return args, disabled

def RunTests(tests, args):
    global logger
    logger = open(args.log_file, 'w')
    success = 0
    fail = 0
    index = 0
    progress = 0
    milestones = []
    for i in range(20):
        milestones.append(int(len(tests)/20*(i+1)))

    for test in tests:
        result = RunTest(test['name'], args.executable, args.timeout, test['table'], test['commands'], args.log_success, test['delimiter'], test['isError'])
        index = index + 1
        if index in milestones:
            progress = progress + 5
            print('[' + str(progress) + '%]')
        if result:
            success = success + 1
        else:
            fail = fail + 1
   
    print("DONE")
    print('------ SUMMARY ------')
    print('PASSED: ' + str(success))
    print('FAILED: ' + str(fail))
    logger.close()
    logger = None

# Main
signal.signal(signal.SIGALRM, AlarmHandle)
args, disabled = ParseArgs()
print("Preparing tests ...")
tests = CreateTests(disabled)
print("DONE")
print("Running tests ...")
RunTests(tests, args)
