from enum import Enum

class TokenType(Enum):
    RAW = 1

    # literals
    INT_LITERAL = 2
    STR_LITERAL = 3
    CHAR_LITERAL = 17
    FLOAT_LITERAL = 4

    # identifiers
    IDENTYPE = 5

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
    
    def get_str(self, ref: str):
        return f"({self.type} '{ref[self.pos:self.pos+self.len]}')"
    
    def __str__(self):
        return f"({self.type} pos={self.pos} len={self.len})"
    
    def __repr__(self):
        return self.__str__()

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

# strings or chars do not contain their end markers.
def isolate_strings_and_chars(tokens: list[Token], ref: str) -> list[Token]:

    out: list[Token] = []

    _pos = 0

    for token in tokens:
        if token.type != TokenType.RAW:
            out.append(token)
            continue

        state = 1
        _pos = token.pos

        # this is a subset of the comment-string state machine
        for i in range(token.pos, token.pos + token.len):
            c = ref[i]
            match state:
                # not in a comment
                case 1:
                    if c == '\'':
                        state = 6
                        out.append(Token(TokenType.RAW, _pos, i - _pos))
                        _pos = i + 1
                    elif c == '"':
                        state = 7
                        out.append(Token(TokenType.RAW, _pos, i - _pos))
                        _pos = i + 1
                
                # in a 'string'
                case 6:
                    if c == '\\':
                        state = 9
                    elif c == '\'':
                        state = 1
                        out.append(Token(TokenType.CHAR_LITERAL, _pos, i - _pos))
                        _pos = i + 1

                # in a "string"
                case 7:
                    if c == '\\':
                        state = 8
                    elif c == '"':
                        state = 1
                        out.append(Token(TokenType.STR_LITERAL, _pos, i - _pos))
                        _pos = i + 1
                
                # backslashed in 'string', ignore function of following character
                case 8:
                    state = 7
                
                # backslashed in "string", ||
                case 9:
                    state = 6

    out.append(Token(TokenType.RAW, _pos, len(ref) - _pos))

    return out

AZ = 'abcdefghijklmnopqrstuvwxyz'
AZ = AZ + AZ.upper() + "_"
AZ3 = AZ + '0123456789'

def isolate_identypes(tokens: list[Token], ref: str):

    out: list[Token] = []

    for token in tokens:
        if token.type != TokenType.RAW:
            out.append(token)
            continue

        state = 0
        _pos = token.pos
        
        for i in range(token.pos, token.pos + token.len):
            c = ref[i]

            match (state):
                # initial state
                case 0:
                    if c in AZ:
                        state = 1
                    else:
                        state = 2
                
                # we are inside an area that isn't an identype.
                case 1:
                    if c not in AZ3:
                        state = 2
                
                # we are inside an area that is an identype.
                case 2:
                    if c in AZ3:
                        continue
                    else:
                        state = 1
                        out.append(Token(TokenType.IDENTYPE, _pos, i - _pos))
                        _pos = i + 1

def create_empty_token_list(ref: str) -> list[Token]:
    return [Token(TokenType.RAW, 0, len(ref))]

def print_token_list(tokens: list[Token], ref: str):
    for token in tokens:
        print("="*80)
        print(token)
        print(ref[token.pos:token.pos + token.len])

def main():
    file = "stage1.c"

    with open(file, "r") as f:
        # I call these spaces telomeres. They will get lost. 
        ref = "  " + f.read() + "  " 

    ref = remove_comments(ref)
    tokens = create_empty_token_list(ref)

    tokens = isolate_strings_and_chars(tokens, ref)
    # isolate_identypes(tokens)
    
    print_token_list(tokens, ref)

if __name__ == "__main__":
    main()