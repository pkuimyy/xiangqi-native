declare const WHITE = "w";
declare const BLACK = "b";
type Color = 'w' | 'b';
declare const KING = "k";
declare const ADVISOR = "a";
declare const ELEPHANT = "b";
declare const HORSE = "n";
declare const ROOK = "r";
declare const CANNON = "c";
declare const PAWN = "p";
type PieceSymbol = 'p' | 'n' | 'b' | 'r' | 'c' | 'k' | 'a';
type Square = 'a9' | 'b9' | 'c9' | 'd9' | 'e9' | 'f9' | 'g9' | 'h9' | 'i9' | 'a8' | 'b8' | 'c8' | 'd8' | 'e8' | 'f8' | 'g8' | 'h8' | 'i8' | 'a7' | 'b7' | 'c7' | 'd7' | 'e7' | 'f7' | 'g7' | 'h7' | 'i7' | 'a6' | 'b6' | 'c6' | 'd6' | 'e6' | 'f6' | 'g6' | 'h6' | 'i6' | 'a5' | 'b5' | 'c5' | 'd5' | 'e5' | 'f5' | 'g5' | 'h5' | 'i5' | 'a4' | 'b4' | 'c4' | 'd4' | 'e4' | 'f4' | 'g4' | 'h4' | 'i4' | 'a3' | 'b3' | 'c3' | 'd3' | 'e3' | 'f3' | 'g3' | 'h3' | 'i3' | 'a2' | 'b2' | 'c2' | 'd2' | 'e2' | 'f2' | 'g2' | 'h2' | 'i2' | 'a1' | 'b1' | 'c1' | 'd1' | 'e1' | 'f1' | 'g1' | 'h1' | 'i1' | 'a0' | 'b0' | 'c0' | 'd0' | 'e0' | 'f0' | 'g0' | 'h0' | 'i0';
type Piece = {
    color: Color;
    type: PieceSymbol;
};
type InternalMove = {
    color: Color;
    from: number;
    to: number;
    piece: PieceSymbol;
    captured?: PieceSymbol;
    flags: number;
};
declare const DEFAULT_POSITION = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1";
declare const SQUARES: Square[];
declare const SUFFIX_LIST: readonly ["!", "?", "!!", "!?", "?!", "??"];
type Suffix = (typeof SUFFIX_LIST)[number];
declare const NAG_TO_SYMBOL: {
    readonly 7: "□";
    readonly 22: "⨀";
    readonly 10: "=";
    readonly 13: "∞";
    readonly 14: "⩲";
    readonly 15: "⩱";
    readonly 16: "±";
    readonly 17: "∓";
    readonly 18: "+{}-";
    readonly 19: "-+";
    readonly 146: "N";
    readonly 32: "↑↑";
    readonly 36: "↑";
    readonly 40: "→";
    readonly 132: "⇆";
    readonly 138: "⊕";
    readonly 44: "=∞";
    readonly 140: "∆";
};
type NAG = number;
/**
 * Convert a NAG (Numeric Annotation Glyph) to its text symbol representation.
 */
declare function nagToGlyph(nag: NAG): string | undefined;
declare const SEVEN_TAG_ROSTER: Record<string, string>;

/**
 * @license
 * Copyright (c) 2025, Jeff Hlywa (jhlywa@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

declare class Move {
    color: Color;
    from: Square;
    to: Square;
    piece: PieceSymbol;
    captured?: PieceSymbol;
    flags: string;
    zh: string;
    lan: string;
    before: string;
    after: string;
    wxf: string;
    iccs: string;
    isCheck: boolean;
    isCheckmate: boolean;
    constructor(internal: InternalMove, zh: string, before: string, after: string, wxf: string, iccs: string, isCheck?: boolean, isCheckmate?: boolean);
    isCapture(): boolean;
    isNullMove(): boolean;
}

/**
 * @license
 * Copyright (c) 2025, Jeff Hlywa (jhlywa@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

declare function xoroshiro128(state: bigint): () => bigint;

/**
 * @license
 * Copyright (c) 2025, Jeff Hlywa (jhlywa@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

declare function validateFen(fen: string): {
    ok: boolean;
    error?: string;
};

/**
 * @license
 * Copyright (c) 2025, Jeff Hlywa (jhlywa@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

declare class Chess {
    private _board;
    private _turn;
    private _header;
    private _kings;
    private _halfMoves;
    private _moveNumber;
    private _history;
    private _comments;
    private _suffixes;
    private _nags;
    private _hash;
    private _positionCount;
    private _isCheck;
    private _isCheckmate;
    constructor(fen?: string, { skipValidation }?: {
        skipValidation?: boolean | undefined;
    });
    clear({ preserveHeaders }?: {
        preserveHeaders?: boolean | undefined;
    }): void;
    load(fen: string, { skipValidation, preserveHeaders }?: {
        skipValidation?: boolean | undefined;
        preserveHeaders?: boolean | undefined;
    }): void;
    fen(): string;
    private _updateSetup;
    reset(): void;
    get(square: Square): Piece | undefined;
    findPiece(piece: Piece): Square[];
    put({ type, color }: {
        type: PieceSymbol;
        color: Color;
    }, square: Square): boolean;
    private _set;
    private _put;
    private _clear;
    remove(square: Square): Piece | undefined;
    attackers(square: Square, attackedBy?: Color): Square[];
    hash(): string;
    isAttacked(square: Square, attackedBy: Color): boolean;
    isCheck(): boolean;
    inCheck(): boolean;
    isCheckmate(): boolean;
    isStalemate(): boolean;
    isInsufficientMaterial(): boolean;
    isThreefoldRepetition(): boolean;
    isDrawByFiftyMoves(): boolean;
    isDraw(): boolean;
    isGameOver(): boolean;
    private _createMove;
    moves(): string[];
    moves({ square }: {
        square: Square;
    }): string[];
    moves({ piece }: {
        piece: PieceSymbol;
    }): string[];
    moves({ square, piece }: {
        square: Square;
        piece: PieceSymbol;
    }): string[];
    moves({ chinese, square }: {
        chinese: true;
        square?: Square;
    }): string[];
    moves({ chinese, square }: {
        chinese?: boolean;
        square?: Square;
    }): string[];
    moves({ chinese, piece }: {
        chinese: true;
        piece?: PieceSymbol;
    }): string[];
    moves({ chinese, piece, }: {
        chinese?: boolean;
        piece?: PieceSymbol;
    }): string[];
    moves({ chinese, square, piece, }: {
        chinese: true;
        square?: Square;
        piece?: PieceSymbol;
    }): string[];
    moves({ chinese, square, piece, }: {
        chinese?: boolean;
        square?: Square;
        piece?: PieceSymbol;
    }): string[];
    moves({ verbose, square }: {
        verbose: true;
        square?: Square;
    }): Move[];
    moves({ verbose, square }: {
        verbose: false;
        square?: Square;
    }): string[];
    moves({ verbose, square, }: {
        verbose?: boolean;
        square?: Square;
    }): string[] | Move[];
    moves({ verbose, piece }: {
        verbose: true;
        piece?: PieceSymbol;
    }): Move[];
    moves({ verbose, piece }: {
        verbose: false;
        piece?: PieceSymbol;
    }): string[];
    moves({ verbose, piece, }: {
        verbose?: boolean;
        piece?: PieceSymbol;
    }): string[] | Move[];
    moves({ verbose, square, piece, }: {
        verbose: true;
        square?: Square;
        piece?: PieceSymbol;
    }): Move[];
    moves({ verbose, square, piece, }: {
        verbose: false;
        square?: Square;
        piece?: PieceSymbol;
    }): string[];
    moves({ verbose, square, piece, }: {
        verbose?: boolean;
        square?: Square;
        piece?: PieceSymbol;
    }): string[] | Move[];
    moves({ square, piece }: {
        square?: Square;
        piece?: PieceSymbol;
    }): Move[];
    private _moves;
    move(move: string | {
        from: string;
        to: string;
    } | null, { strict }?: {
        strict?: boolean;
    }): Move;
    private _push;
    private _movePiece;
    private _makeMove;
    undo(): Move | null;
    private _undoMove;
    pgn({ newline, maxWidth, }?: {
        newline?: string;
        maxWidth?: number;
    }): string;
    header(...args: string[]): Record<string, string | null>;
    setHeader(key: string, value: string): Record<string, string>;
    removeHeader(key: string): boolean;
    getHeaders(): Record<string, string>;
    loadPgn(pgn: string, { strict, newlineChar, }?: {
        strict?: boolean;
        newlineChar?: string;
    }): void;
    ascii(): string;
    perft(depth: number): number;
    setTurn(color: Color): boolean;
    turn(): Color;
    board(): ({
        square: Square;
        type: PieceSymbol;
        color: Color;
    } | null)[][];
    squareColor(square: Square): 'light' | 'dark' | null;
    history(): string[];
    history({ verbose }: {
        verbose: true;
    }): Move[];
    history({ verbose }: {
        verbose: false;
    }): string[];
    history({ verbose }: {
        verbose: boolean;
    }): string[] | Move[];
    private _getPositionCount;
    private _incPositionCount;
    private _decPositionCount;
    private _pruneComments;
    getComment(): string;
    setComment(comment: string): void;
    deleteComment(): string;
    removeComment(): string;
    getComments(): {
        fen: string;
        comment?: string;
        suffixAnnotation?: string;
        nags: NAG[];
    }[];
    getSuffixAnnotation(fen?: string): Suffix | undefined;
    setSuffixAnnotation(suffix: Suffix, fen?: string): void;
    removeSuffixAnnotation(fen?: string): Suffix | undefined;
    getNags(fen?: string): NAG[];
    addNag(nag: NAG, fen?: string): void;
    setNags(nags: NAG[], fen?: string): void;
    removeNags(fen?: string): NAG[];
    removeNag(nag: NAG, fen?: string): boolean;
    deleteComments(): {
        fen: string;
        comment: string;
    }[];
    removeComments(): {
        fen: string;
        comment: string;
    }[];
    moveNumber(): number;
}

export { ADVISOR, BLACK, CANNON, Chess, DEFAULT_POSITION, ELEPHANT, HORSE, KING, Move, NAG_TO_SYMBOL, PAWN, ROOK, SEVEN_TAG_ROSTER, SQUARES, SUFFIX_LIST, WHITE, nagToGlyph, validateFen, xoroshiro128 };
export type { Color, NAG, Piece, PieceSymbol, Square, Suffix };
