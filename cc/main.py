from enum import Enum

class TokenType(Enum):
    RAW = 1

    # literals
    INT_LITERAL = 2
    STR_LITERAL = 3
    FLOAT_LITERAL = 4

    # identifiers
    IDENTIFIER_OR_TYPE = 5

    # symbols
    PLUS = 6
    MINUS = 7
    STAR = 8
    DIVIDE = 9
    EQUALS = 10
    
    # parentheses
    LEFTB = 11 # []
    RIGHTB = 12
    LEFTP = 13 # ()
    RIGHTP = 14
    LEFTC = 15 # {}
    RIGHTC = 16

class Token:
    def __init__(self, _type: TokenType, _pos: int, _len: int):
        self.type = _type
        self.pos = _pos
        self.len = _len

class TokenList:
    def __init__(self, _data: str):
        self.tokens = [Token(TokenType.RAW, 0, len(_data))]
        self.ref = _data

def remove_comments(data: str) -> str:
    out = ""

    state = 1

    for c in data:
        match state:
            # not in a comment
            case 1:
                if c == '/':
                    state = 2
                elif c == '\'':
                    out += c
                    state = 6
                elif c == '"':
                    out += c
                    state = 7
                else:
                    out += c
            
            # a slash, maybe it's a comment?
            case 2:
                if c == '/':
                    state = 3
                elif c == '*':
                    state = 4
                else:
                    state = 1
                    out += '/' # it wasn't, put the slash back
                    out += c

            # in a single line comment
            case 3:
                if c == '\n':
                    out += '\n'
                    state = 1
            
            # in a multi-line comment
            case 4:
                if c == '*':
                    state = 5
            
            # in a multi-line comment, saw an asterisk, is it closing?
            case 5:
                if c == '/':
                    out += '\n'
                    state = 1
                else:
                    state = 4
            
            # in a 'string'
            case 6:
                if c == '\\':
                    out += c
                    state = 9
                elif c == '\'':
                    out += c
                    state = 1
                else:
                    out += c

            # in a "string"
            case 7:
                if c == '\\':
                    out += c
                    state = 8
                elif c == '"':
                    out += c
                    state = 1
                else:
                    out += c
            
            # backslashed in 'string', ignore function of following character
            case 8:
                out += c
                state = 7
            
            # backslashed in "string", ||
            case 9:
                out += c
                state = 6
    
    if state != 1:
        raise SyntaxError("unclosed comment or string")

    return out

def init_tokens(data: str) -> TokenList:
    return TokenList(data)


def main():
    print("Welcome to PbO2!")

    file = "stage1.c"

    with open(file, "r") as f:
        data = f.read()

    data = remove_comments(data)
    tokens = init_tokens(data)
    

    print(tokens)

    print("done")

if __name__ == "__main__":
    main()