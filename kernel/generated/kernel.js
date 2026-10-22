(() => {
  // kernel/upstream/lib/Tree/layout.ts
  function buildWNode(node2, parent, foldedNodes) {
    const wn = {
      node: node2,
      parent,
      children: [],
      depth: parent === null ? 0 : parent.depth + 1,
      x: 0,
      y: 0,
      relX: 0,
      prelim: 0,
      shift: 0,
      change: 0,
      lExt: null,
      rExt: null,
      lExtRelX: 0,
      rExtRelX: 0,
      lThr: null,
      rThr: null
    };
    wn.lExt = wn;
    wn.rExt = wn;
    for (let i = 0; i < node2.children.length; i++) {
      const child = node2.children[i];
      if (i > 0 && foldedNodes.has(node2.id)) continue;
      wn.children.push(buildWNode(child, wn, foldedNodes));
    }
    return wn;
  }
  var NODE_H = 1;
  function nodeBottom(w) {
    return w.y + (w.children.length === 0 ? 2 : 1);
  }
  function layoutChildren(w, y, spacingFn) {
    w.y = y;
    if (w.depth % 2 === 0 && w.children.length > 1) {
      w.children.reverse();
    }
    const kids = w.children;
    let lastLows = null;
    for (let i = 0; i < kids.length; i++) {
      const kid = kids[i];
      layoutChildren(kid, w.y + NODE_H, spacingFn);
      const lowY = i === 0 ? nodeBottom(kid.lExt) : nodeBottom(kid.rExt);
      if (i !== 0) separate(w, i, lastLows, spacingFn);
      lastLows = updateLows(lowY, i, lastLows);
    }
    shiftChange(w);
    positionRoot(w);
  }
  function separate(w, i, lows, spacingFn) {
    const lSib = w.children[i - 1];
    const cur = w.children[i];
    let rContour = lSib;
    let rSumMods = lSib.relX;
    let lContour = cur;
    let lSumMods = cur.relX;
    let isFirst = true;
    while (rContour && lContour) {
      if (nodeBottom(rContour) > lows.lowY) lows = lows.next;
      const dist = rSumMods + rContour.prelim - (lSumMods + lContour.prelim) + 0.5 + 0.5 + // xSize/2 + xSize/2 = 1/2 + 1/2
      spacingFn(rContour, lContour);
      if (dist > 0 || dist < 0 && isFirst) {
        lSumMods += dist;
        moveSubtree(cur, dist);
        distributeExtra(w, i, lows.index, dist);
      }
      isFirst = false;
      const rBottom = nodeBottom(rContour);
      const lBottom = nodeBottom(lContour);
      if (rBottom <= lBottom) {
        rContour = nextRContour(rContour);
        if (rContour) rSumMods += rContour.relX;
      }
      if (rBottom >= lBottom) {
        lContour = nextLContour(lContour);
        if (lContour) lSumMods += lContour.relX;
      }
    }
    if (!rContour && lContour) setLThr(w, i, lContour, lSumMods);
    else if (rContour && !lContour) setRThr(w, i, rContour, rSumMods);
  }
  function moveSubtree(subtree, dist) {
    subtree.relX += dist;
    subtree.lExtRelX += dist;
    subtree.rExtRelX += dist;
  }
  function distributeExtra(w, curI, leftI, dist) {
    const n = curI - leftI;
    if (n > 1) {
      const delta = dist / n;
      w.children[leftI + 1].shift += delta;
      w.children[curI].shift -= delta;
      w.children[curI].change -= dist - delta;
    }
  }
  function shiftChange(w) {
    const kids = w.children;
    let lastShiftSum = 0, lastChangeSum = 0;
    for (const c of kids) {
      const shiftSum = lastShiftSum + c.shift;
      const changeSum = lastChangeSum + shiftSum + c.change;
      c.relX += changeSum;
      lastShiftSum = shiftSum;
      lastChangeSum = changeSum;
    }
  }
  function positionRoot(w) {
    if (w.children.length === 0) return;
    const isEven = w.depth % 2 === 0;
    const kAlign = isEven ? w.children[w.children.length - 1] : w.children[0];
    const kf = w.children[w.children.length - 1];
    w.prelim = kAlign.prelim + kAlign.relX;
    w.lExt = isEven ? w.children[0].lExt : kAlign.lExt;
    w.lExtRelX = isEven ? w.children[0].lExtRelX : kAlign.lExtRelX;
    w.rExt = kf.rExt;
    w.rExtRelX = kf.rExtRelX;
  }
  function nextLContour(w) {
    if (w.children.length > 0 && w.children[0] !== w.lThr) return w.children[0];
    return w.lThr;
  }
  function nextRContour(w) {
    if (w.children.length > 0 && w.children[w.children.length - 1] !== w.rThr)
      return w.children[w.children.length - 1];
    return w.rThr;
  }
  function setLThr(w, i, lContour, lSumMods) {
    const f = w.children[0];
    const lExt = f.lExt;
    const cur = w.children[i];
    lExt.lThr = lContour;
    const diff = lSumMods - lContour.relX - f.lExtRelX;
    lExt.relX += diff;
    lExt.prelim -= diff;
    f.lExt = cur.lExt;
    f.lExtRelX = cur.lExtRelX;
  }
  function setRThr(w, i, rContour, rSumMods) {
    const cur = w.children[i];
    const rExt = cur.rExt;
    const lSib = w.children[i - 1];
    rExt.rThr = rContour;
    const diff = rSumMods - rContour.relX - cur.rExtRelX;
    rExt.relX += diff;
    rExt.prelim -= diff;
    cur.rExt = lSib.rExt;
    cur.rExtRelX = lSib.rExtRelX;
  }
  function updateLows(lowY, index, lastLows) {
    while (lastLows !== null && lowY >= lastLows.lowY) lastLows = lastLows.next;
    return { lowY, index, next: lastLows };
  }
  function resolveX(w, prevSum, parentX) {
    if (prevSum === void 0) {
      prevSum = -w.relX - w.prelim;
      parentX = 0;
    }
    const pX = parentX;
    const sum = prevSum + w.relX;
    w.relX = sum + w.prelim - pX;
    w.prelim = 0;
    w.x = pX + w.relX;
    for (const kid of w.children) {
      resolveX(kid, sum, w.x);
    }
  }
  function calculateTreeLayout(nodeMap, foldedNodes, spacing = 0.3) {
    const root = nodeMap.get("node-root");
    if (!root) return [];
    const wRoot = buildWNode(root, null, foldedNodes);
    layoutChildren(wRoot, 0, () => spacing);
    resolveX(wRoot);
    const result = [];
    function collect(wn) {
      wn.node.x = Math.round(wn.x * 10) / 10;
      wn.node.y = wn.depth;
      result.push(wn.node);
      for (const child of wn.children) {
        collect(child);
      }
    }
    collect(wRoot);
    return result;
  }

  // reference/xiangqi.js/dist/esm/chess.js
  function rootNode(comment) {
    return comment !== null ? { comment, nags: [], variations: [] } : { nags: [], variations: [] };
  }
  function node(move, suffixAnnotation, nags, comment, variations) {
    const node2 = { move, nags, variations };
    if (suffixAnnotation) {
      node2.suffixAnnotation = suffixAnnotation;
    }
    if (comment !== null) {
      node2.comment = comment;
    }
    return node2;
  }
  function lineToTree(...nodes) {
    const [root, ...rest] = nodes;
    let parent = root;
    for (const child of rest) {
      if (child !== null) {
        parent.variations = [child, ...child.variations];
        child.variations = [];
        parent = child;
      }
    }
    return root;
  }
  function pgn(headers, game) {
    if (game.marker && game.marker.comment) {
      let node2 = game.root;
      while (true) {
        const next = node2.variations[0];
        if (!next) {
          node2.comment = game.marker.comment;
          break;
        }
        node2 = next;
      }
    }
    return {
      headers,
      root: game.root,
      result: (game.marker && game.marker.result) ?? void 0
    };
  }
  function peg$subclass(child, parent) {
    function C() {
      this.constructor = child;
    }
    C.prototype = parent.prototype;
    child.prototype = new C();
  }
  function peg$SyntaxError(message, expected, found, location) {
    var self = Error.call(this, message);
    if (Object.setPrototypeOf) {
      Object.setPrototypeOf(self, peg$SyntaxError.prototype);
    }
    self.expected = expected;
    self.found = found;
    self.location = location;
    self.name = "SyntaxError";
    return self;
  }
  peg$subclass(peg$SyntaxError, Error);
  function peg$padEnd(str, targetLength, padString) {
    padString = padString || " ";
    if (str.length > targetLength) {
      return str;
    }
    targetLength -= str.length;
    padString += padString.repeat(targetLength);
    return str + padString.slice(0, targetLength);
  }
  peg$SyntaxError.prototype.format = function(sources) {
    var str = "Error: " + this.message;
    if (this.location) {
      var src = null;
      var k;
      for (k = 0; k < sources.length; k++) {
        if (sources[k].source === this.location.source) {
          src = sources[k].text.split(/\r\n|\n|\r/g);
          break;
        }
      }
      var s = this.location.start;
      var offset_s = this.location.source && typeof this.location.source.offset === "function" ? this.location.source.offset(s) : s;
      var loc = this.location.source + ":" + offset_s.line + ":" + offset_s.column;
      if (src) {
        var e = this.location.end;
        var filler = peg$padEnd("", offset_s.line.toString().length, " ");
        var line = src[s.line - 1];
        var last = s.line === e.line ? e.column : line.length + 1;
        var hatLen = last - s.column || 1;
        str += "\n --> " + loc + "\n" + filler + " |\n" + offset_s.line + " | " + line + "\n" + filler + " | " + peg$padEnd("", s.column - 1, " ") + peg$padEnd("", hatLen, "^");
      } else {
        str += "\n at " + loc;
      }
    }
    return str;
  };
  peg$SyntaxError.buildMessage = function(expected, found) {
    var DESCRIBE_EXPECTATION_FNS = {
      literal: function(expectation) {
        return '"' + literalEscape(expectation.text) + '"';
      },
      class: function(expectation) {
        var escapedParts = expectation.parts.map(function(part) {
          return Array.isArray(part) ? classEscape(part[0]) + "-" + classEscape(part[1]) : classEscape(part);
        });
        return "[" + (expectation.inverted ? "^" : "") + escapedParts.join("") + "]";
      },
      any: function() {
        return "any character";
      },
      end: function() {
        return "end of input";
      },
      other: function(expectation) {
        return expectation.description;
      }
    };
    function hex(ch) {
      return ch.charCodeAt(0).toString(16).toUpperCase();
    }
    function literalEscape(s) {
      return s.replace(/\\/g, "\\\\").replace(/"/g, '\\"').replace(/\0/g, "\\0").replace(/\t/g, "\\t").replace(/\n/g, "\\n").replace(/\r/g, "\\r").replace(/[\x00-\x0F]/g, function(ch) {
        return "\\x0" + hex(ch);
      }).replace(/[\x10-\x1F\x7F-\x9F]/g, function(ch) {
        return "\\x" + hex(ch);
      });
    }
    function classEscape(s) {
      return s.replace(/\\/g, "\\\\").replace(/\]/g, "\\]").replace(/\^/g, "\\^").replace(/-/g, "\\-").replace(/\0/g, "\\0").replace(/\t/g, "\\t").replace(/\n/g, "\\n").replace(/\r/g, "\\r").replace(/[\x00-\x0F]/g, function(ch) {
        return "\\x0" + hex(ch);
      }).replace(/[\x10-\x1F\x7F-\x9F]/g, function(ch) {
        return "\\x" + hex(ch);
      });
    }
    function describeExpectation(expectation) {
      return DESCRIBE_EXPECTATION_FNS[expectation.type](expectation);
    }
    function describeExpected(expected2) {
      var descriptions = expected2.map(describeExpectation);
      var i, j;
      descriptions.sort();
      if (descriptions.length > 0) {
        for (i = 1, j = 1; i < descriptions.length; i++) {
          if (descriptions[i - 1] !== descriptions[i]) {
            descriptions[j] = descriptions[i];
            j++;
          }
        }
        descriptions.length = j;
      }
      switch (descriptions.length) {
        case 1:
          return descriptions[0];
        case 2:
          return descriptions[0] + " or " + descriptions[1];
        default:
          return descriptions.slice(0, -1).join(", ") + ", or " + descriptions[descriptions.length - 1];
      }
    }
    function describeFound(found2) {
      return found2 ? '"' + literalEscape(found2) + '"' : "end of input";
    }
    return "Expected " + describeExpected(expected) + " but " + describeFound(found) + " found.";
  };
  function peg$parse(input, options) {
    options = options !== void 0 ? options : {};
    var peg$FAILED = {};
    var peg$source = options.grammarSource;
    var peg$startRuleFunctions = { pgn: peg$parsepgn };
    var peg$startRuleFunction = peg$parsepgn;
    var peg$c0 = "[";
    var peg$c1 = '"';
    var peg$c2 = "]";
    var peg$c3 = ".";
    var peg$c4 = "-";
    var peg$c5 = "$";
    var peg$c6 = "{";
    var peg$c7 = "}";
    var peg$c8 = ";";
    var peg$c9 = "(";
    var peg$c10 = ")";
    var peg$c11 = "1-0";
    var peg$c12 = "0-1";
    var peg$c13 = "1/2-1/2";
    var peg$c14 = "*";
    var peg$r0 = /^[a-zA-Z]/;
    var peg$r1 = /^[^"]/;
    var peg$r2 = /^[0-9]/;
    var peg$r3 = /^[.]/;
    var peg$r4 = /^[+=.x\-]/;
    var peg$r5 = /^[+#]/;
    var peg$r6 = /^[+\-]/;
    var peg$r7 = /^[KABENHRCPkabenhrcp]/;
    var peg$r8 = /^[1-9]/;
    var peg$r9 = /^[Pp]/;
    var peg$r10 = /^[a-i]/;
    var peg$r11 = /^[!?]/;
    var peg$r12 = /^[^}]/;
    var peg$r13 = /^[^\r\n]/;
    var peg$r14 = /^[ \t\r\n]/;
    var peg$e0 = peg$otherExpectation("tag pair");
    var peg$e1 = peg$literalExpectation("[", false);
    var peg$e2 = peg$literalExpectation('"', false);
    var peg$e3 = peg$literalExpectation("]", false);
    var peg$e4 = peg$otherExpectation("tag name");
    var peg$e5 = peg$classExpectation([["a", "z"], ["A", "Z"]], false, false);
    var peg$e6 = peg$otherExpectation("tag value");
    var peg$e7 = peg$classExpectation(['"'], true, false);
    var peg$e8 = peg$otherExpectation("move number");
    var peg$e9 = peg$classExpectation([["0", "9"]], false, false);
    var peg$e10 = peg$literalExpectation(".", false);
    var peg$e11 = peg$classExpectation(["."], false, false);
    var peg$e12 = peg$otherExpectation("Chinese chess algebraic notation");
    var peg$e13 = peg$classExpectation(["+", "=", ".", "x", "-"], false, false);
    var peg$e14 = peg$classExpectation(["+", "#"], false, false);
    var peg$e15 = peg$classExpectation(["+", "-"], false, false);
    var peg$e16 = peg$classExpectation(["K", "A", "B", "E", "N", "H", "R", "C", "P", "k", "a", "b", "e", "n", "h", "r", "c", "p"], false, false);
    var peg$e17 = peg$classExpectation([["1", "9"]], false, false);
    var peg$e18 = peg$classExpectation(["P", "p"], false, false);
    var peg$e19 = peg$classExpectation([["a", "i"]], false, false);
    var peg$e20 = peg$literalExpectation("-", false);
    var peg$e21 = peg$otherExpectation("suffix annotation");
    var peg$e22 = peg$classExpectation(["!", "?"], false, false);
    var peg$e23 = peg$otherExpectation("NAG");
    var peg$e24 = peg$literalExpectation("$", false);
    var peg$e25 = peg$otherExpectation("brace comment");
    var peg$e26 = peg$literalExpectation("{", false);
    var peg$e27 = peg$classExpectation(["}"], true, false);
    var peg$e28 = peg$literalExpectation("}", false);
    var peg$e29 = peg$otherExpectation("rest of line comment");
    var peg$e30 = peg$literalExpectation(";", false);
    var peg$e31 = peg$classExpectation(["\r", "\n"], true, false);
    var peg$e32 = peg$otherExpectation("variation");
    var peg$e33 = peg$literalExpectation("(", false);
    var peg$e34 = peg$literalExpectation(")", false);
    var peg$e35 = peg$otherExpectation("game termination marker");
    var peg$e36 = peg$literalExpectation("1-0", false);
    var peg$e37 = peg$literalExpectation("0-1", false);
    var peg$e38 = peg$literalExpectation("1/2-1/2", false);
    var peg$e39 = peg$literalExpectation("*", false);
    var peg$e40 = peg$otherExpectation("whitespace");
    var peg$e41 = peg$classExpectation([" ", "	", "\r", "\n"], false, false);
    var peg$f0 = function(headers, game) {
      return pgn(headers, game);
    };
    var peg$f1 = function(tagPairs) {
      return Object.fromEntries(tagPairs);
    };
    var peg$f2 = function(tagName, tagValue) {
      return [tagName, tagValue];
    };
    var peg$f3 = function(root, marker) {
      return { root, marker };
    };
    var peg$f4 = function(comment, moves) {
      return lineToTree(rootNode(comment), ...moves.flat());
    };
    var peg$f5 = function(san, suffixAnnotation, nags, comment, variations) {
      return node(san, suffixAnnotation, nags, comment, variations);
    };
    var peg$f6 = function(nag) {
      return parseInt(nag, 10);
    };
    var peg$f7 = function(comment) {
      return comment.replace(/[\r\n]+/g, " ");
    };
    var peg$f8 = function(comment) {
      return comment.trim();
    };
    var peg$f9 = function(line) {
      return line;
    };
    var peg$f10 = function(result, comment) {
      return { result, comment };
    };
    var peg$currPos = options.peg$currPos | 0;
    var peg$posDetailsCache = [{ line: 1, column: 1 }];
    var peg$maxFailPos = peg$currPos;
    var peg$maxFailExpected = options.peg$maxFailExpected || [];
    var peg$silentFails = options.peg$silentFails | 0;
    var peg$result;
    if (options.startRule) {
      if (!(options.startRule in peg$startRuleFunctions)) {
        throw new Error(`Can't start parsing from rule "` + options.startRule + '".');
      }
      peg$startRuleFunction = peg$startRuleFunctions[options.startRule];
    }
    function peg$literalExpectation(text, ignoreCase) {
      return { type: "literal", text, ignoreCase };
    }
    function peg$classExpectation(parts, inverted, ignoreCase) {
      return { type: "class", parts, inverted, ignoreCase };
    }
    function peg$endExpectation() {
      return { type: "end" };
    }
    function peg$otherExpectation(description) {
      return { type: "other", description };
    }
    function peg$computePosDetails(pos) {
      var details = peg$posDetailsCache[pos];
      var p;
      if (details) {
        return details;
      } else {
        if (pos >= peg$posDetailsCache.length) {
          p = peg$posDetailsCache.length - 1;
        } else {
          p = pos;
          while (!peg$posDetailsCache[--p]) {
          }
        }
        details = peg$posDetailsCache[p];
        details = {
          line: details.line,
          column: details.column
        };
        while (p < pos) {
          if (input.charCodeAt(p) === 10) {
            details.line++;
            details.column = 1;
          } else {
            details.column++;
          }
          p++;
        }
        peg$posDetailsCache[pos] = details;
        return details;
      }
    }
    function peg$computeLocation(startPos, endPos, offset) {
      var startPosDetails = peg$computePosDetails(startPos);
      var endPosDetails = peg$computePosDetails(endPos);
      var res = {
        source: peg$source,
        start: {
          offset: startPos,
          line: startPosDetails.line,
          column: startPosDetails.column
        },
        end: {
          offset: endPos,
          line: endPosDetails.line,
          column: endPosDetails.column
        }
      };
      return res;
    }
    function peg$fail(expected) {
      if (peg$currPos < peg$maxFailPos) {
        return;
      }
      if (peg$currPos > peg$maxFailPos) {
        peg$maxFailPos = peg$currPos;
        peg$maxFailExpected = [];
      }
      peg$maxFailExpected.push(expected);
    }
    function peg$buildStructuredError(expected, found, location) {
      return new peg$SyntaxError(
        peg$SyntaxError.buildMessage(expected, found),
        expected,
        found,
        location
      );
    }
    function peg$parsepgn() {
      var s0, s1, s2;
      s0 = peg$currPos;
      s1 = peg$parsetagPairSection();
      s2 = peg$parsemoveTextSection();
      s0 = peg$f0(s1, s2);
      return s0;
    }
    function peg$parsetagPairSection() {
      var s0, s1, s2;
      s0 = peg$currPos;
      s1 = [];
      s2 = peg$parsetagPair();
      while (s2 !== peg$FAILED) {
        s1.push(s2);
        s2 = peg$parsetagPair();
      }
      s2 = peg$parse_();
      s0 = peg$f1(s1);
      return s0;
    }
    function peg$parsetagPair() {
      var s0, s2, s4, s6, s7, s8, s10;
      peg$silentFails++;
      s0 = peg$currPos;
      peg$parse_();
      if (input.charCodeAt(peg$currPos) === 91) {
        s2 = peg$c0;
        peg$currPos++;
      } else {
        s2 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e1);
        }
      }
      if (s2 !== peg$FAILED) {
        peg$parse_();
        s4 = peg$parsetagName();
        if (s4 !== peg$FAILED) {
          peg$parse_();
          if (input.charCodeAt(peg$currPos) === 34) {
            s6 = peg$c1;
            peg$currPos++;
          } else {
            s6 = peg$FAILED;
            if (peg$silentFails === 0) {
              peg$fail(peg$e2);
            }
          }
          if (s6 !== peg$FAILED) {
            s7 = peg$parsetagValue();
            if (input.charCodeAt(peg$currPos) === 34) {
              s8 = peg$c1;
              peg$currPos++;
            } else {
              s8 = peg$FAILED;
              if (peg$silentFails === 0) {
                peg$fail(peg$e2);
              }
            }
            if (s8 !== peg$FAILED) {
              peg$parse_();
              if (input.charCodeAt(peg$currPos) === 93) {
                s10 = peg$c2;
                peg$currPos++;
              } else {
                s10 = peg$FAILED;
                if (peg$silentFails === 0) {
                  peg$fail(peg$e3);
                }
              }
              if (s10 !== peg$FAILED) {
                s0 = peg$f2(s4, s7);
              } else {
                peg$currPos = s0;
                s0 = peg$FAILED;
              }
            } else {
              peg$currPos = s0;
              s0 = peg$FAILED;
            }
          } else {
            peg$currPos = s0;
            s0 = peg$FAILED;
          }
        } else {
          peg$currPos = s0;
          s0 = peg$FAILED;
        }
      } else {
        peg$currPos = s0;
        s0 = peg$FAILED;
      }
      peg$silentFails--;
      if (s0 === peg$FAILED) {
        if (peg$silentFails === 0) {
          peg$fail(peg$e0);
        }
      }
      return s0;
    }
    function peg$parsetagName() {
      var s0, s1, s2;
      peg$silentFails++;
      s0 = peg$currPos;
      s1 = [];
      s2 = input.charAt(peg$currPos);
      if (peg$r0.test(s2)) {
        peg$currPos++;
      } else {
        s2 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e5);
        }
      }
      if (s2 !== peg$FAILED) {
        while (s2 !== peg$FAILED) {
          s1.push(s2);
          s2 = input.charAt(peg$currPos);
          if (peg$r0.test(s2)) {
            peg$currPos++;
          } else {
            s2 = peg$FAILED;
            if (peg$silentFails === 0) {
              peg$fail(peg$e5);
            }
          }
        }
      } else {
        s1 = peg$FAILED;
      }
      if (s1 !== peg$FAILED) {
        s0 = input.substring(s0, peg$currPos);
      } else {
        s0 = s1;
      }
      peg$silentFails--;
      if (s0 === peg$FAILED) {
        s1 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e4);
        }
      }
      return s0;
    }
    function peg$parsetagValue() {
      var s0, s1, s2;
      peg$silentFails++;
      s0 = peg$currPos;
      s1 = [];
      s2 = input.charAt(peg$currPos);
      if (peg$r1.test(s2)) {
        peg$currPos++;
      } else {
        s2 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e7);
        }
      }
      while (s2 !== peg$FAILED) {
        s1.push(s2);
        s2 = input.charAt(peg$currPos);
        if (peg$r1.test(s2)) {
          peg$currPos++;
        } else {
          s2 = peg$FAILED;
          if (peg$silentFails === 0) {
            peg$fail(peg$e7);
          }
        }
      }
      s0 = input.substring(s0, peg$currPos);
      peg$silentFails--;
      s1 = peg$FAILED;
      if (peg$silentFails === 0) {
        peg$fail(peg$e6);
      }
      return s0;
    }
    function peg$parsemoveTextSection() {
      var s0, s1, s3;
      s0 = peg$currPos;
      s1 = peg$parseline();
      peg$parse_();
      s3 = peg$parsegameTerminationMarker();
      if (s3 === peg$FAILED) {
        s3 = null;
      }
      peg$parse_();
      s0 = peg$f3(s1, s3);
      return s0;
    }
    function peg$parseline() {
      var s0, s1, s2, s3;
      s0 = peg$currPos;
      s1 = peg$parsecomment();
      if (s1 === peg$FAILED) {
        s1 = null;
      }
      s2 = [];
      s3 = peg$parsemove();
      while (s3 !== peg$FAILED) {
        s2.push(s3);
        s3 = peg$parsemove();
      }
      s0 = peg$f4(s1, s2);
      return s0;
    }
    function peg$parsemove() {
      var s0, s4, s5, s6, s7, s8, s9, s10;
      s0 = peg$currPos;
      peg$parse_();
      peg$parsemoveNumber();
      peg$parse_();
      s4 = peg$parsesan();
      if (s4 !== peg$FAILED) {
        s5 = peg$parsesuffixAnnotation();
        if (s5 === peg$FAILED) {
          s5 = null;
        }
        s6 = [];
        s7 = peg$parsenag();
        while (s7 !== peg$FAILED) {
          s6.push(s7);
          s7 = peg$parsenag();
        }
        s7 = peg$parse_();
        s8 = peg$parsecomment();
        if (s8 === peg$FAILED) {
          s8 = null;
        }
        s9 = [];
        s10 = peg$parsevariation();
        while (s10 !== peg$FAILED) {
          s9.push(s10);
          s10 = peg$parsevariation();
        }
        s0 = peg$f5(s4, s5, s6, s8, s9);
      } else {
        peg$currPos = s0;
        s0 = peg$FAILED;
      }
      return s0;
    }
    function peg$parsemoveNumber() {
      var s0, s1, s2, s3, s4, s5;
      peg$silentFails++;
      s0 = peg$currPos;
      s1 = [];
      s2 = input.charAt(peg$currPos);
      if (peg$r2.test(s2)) {
        peg$currPos++;
      } else {
        s2 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e9);
        }
      }
      while (s2 !== peg$FAILED) {
        s1.push(s2);
        s2 = input.charAt(peg$currPos);
        if (peg$r2.test(s2)) {
          peg$currPos++;
        } else {
          s2 = peg$FAILED;
          if (peg$silentFails === 0) {
            peg$fail(peg$e9);
          }
        }
      }
      if (input.charCodeAt(peg$currPos) === 46) {
        s2 = peg$c3;
        peg$currPos++;
      } else {
        s2 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e10);
        }
      }
      if (s2 !== peg$FAILED) {
        s3 = peg$parse_();
        s4 = [];
        s5 = input.charAt(peg$currPos);
        if (peg$r3.test(s5)) {
          peg$currPos++;
        } else {
          s5 = peg$FAILED;
          if (peg$silentFails === 0) {
            peg$fail(peg$e11);
          }
        }
        while (s5 !== peg$FAILED) {
          s4.push(s5);
          s5 = input.charAt(peg$currPos);
          if (peg$r3.test(s5)) {
            peg$currPos++;
          } else {
            s5 = peg$FAILED;
            if (peg$silentFails === 0) {
              peg$fail(peg$e11);
            }
          }
        }
        s1 = [s1, s2, s3, s4];
        s0 = s1;
      } else {
        peg$currPos = s0;
        s0 = peg$FAILED;
      }
      peg$silentFails--;
      if (s0 === peg$FAILED) {
        s1 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e8);
        }
      }
      return s0;
    }
    function peg$parsesan() {
      var s0, s1;
      peg$silentFails++;
      s0 = peg$currPos;
      s1 = peg$parseWXF_move();
      if (s1 === peg$FAILED) {
        s1 = peg$parseICCS_move();
      }
      if (s1 !== peg$FAILED) {
        s0 = input.substring(s0, peg$currPos);
      } else {
        s0 = s1;
      }
      peg$silentFails--;
      if (s0 === peg$FAILED) {
        s1 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e12);
        }
      }
      return s0;
    }
    function peg$parseWXF_move() {
      var s0, s1, s2, s3, s4;
      s0 = peg$currPos;
      s1 = peg$parsefront_back_piece();
      if (s1 === peg$FAILED) {
        s1 = peg$parsepawn_piece();
        if (s1 === peg$FAILED) {
          s1 = peg$parsepiece_col();
          if (s1 === peg$FAILED) {
            s1 = peg$parsepawn_col();
          }
        }
      }
      if (s1 !== peg$FAILED) {
        s2 = input.charAt(peg$currPos);
        if (peg$r4.test(s2)) {
          peg$currPos++;
        } else {
          s2 = peg$FAILED;
          if (peg$silentFails === 0) {
            peg$fail(peg$e13);
          }
        }
        if (s2 !== peg$FAILED) {
          s3 = input.charAt(peg$currPos);
          if (peg$r2.test(s3)) {
            peg$currPos++;
          } else {
            s3 = peg$FAILED;
            if (peg$silentFails === 0) {
              peg$fail(peg$e9);
            }
          }
          if (s3 !== peg$FAILED) {
            s4 = input.charAt(peg$currPos);
            if (peg$r5.test(s4)) {
              peg$currPos++;
            } else {
              s4 = peg$FAILED;
              if (peg$silentFails === 0) {
                peg$fail(peg$e14);
              }
            }
            if (s4 === peg$FAILED) {
              s4 = null;
            }
            s1 = [s1, s2, s3, s4];
            s0 = s1;
          } else {
            peg$currPos = s0;
            s0 = peg$FAILED;
          }
        } else {
          peg$currPos = s0;
          s0 = peg$FAILED;
        }
      } else {
        peg$currPos = s0;
        s0 = peg$FAILED;
      }
      return s0;
    }
    function peg$parsefront_back_piece() {
      var s0, s1, s2;
      s0 = peg$currPos;
      s1 = input.charAt(peg$currPos);
      if (peg$r6.test(s1)) {
        peg$currPos++;
      } else {
        s1 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e15);
        }
      }
      if (s1 !== peg$FAILED) {
        s2 = input.charAt(peg$currPos);
        if (peg$r7.test(s2)) {
          peg$currPos++;
        } else {
          s2 = peg$FAILED;
          if (peg$silentFails === 0) {
            peg$fail(peg$e16);
          }
        }
        if (s2 !== peg$FAILED) {
          s1 = [s1, s2];
          s0 = s1;
        } else {
          peg$currPos = s0;
          s0 = peg$FAILED;
        }
      } else {
        peg$currPos = s0;
        s0 = peg$FAILED;
      }
      return s0;
    }
    function peg$parsepawn_piece() {
      var s0, s1, s2;
      s0 = peg$currPos;
      s1 = input.charAt(peg$currPos);
      if (peg$r8.test(s1)) {
        peg$currPos++;
      } else {
        s1 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e17);
        }
      }
      if (s1 !== peg$FAILED) {
        s2 = input.charAt(peg$currPos);
        if (peg$r9.test(s2)) {
          peg$currPos++;
        } else {
          s2 = peg$FAILED;
          if (peg$silentFails === 0) {
            peg$fail(peg$e18);
          }
        }
        if (s2 !== peg$FAILED) {
          s1 = [s1, s2];
          s0 = s1;
        } else {
          peg$currPos = s0;
          s0 = peg$FAILED;
        }
      } else {
        peg$currPos = s0;
        s0 = peg$FAILED;
      }
      return s0;
    }
    function peg$parsepiece_col() {
      var s0, s1, s2;
      s0 = peg$currPos;
      s1 = input.charAt(peg$currPos);
      if (peg$r7.test(s1)) {
        peg$currPos++;
      } else {
        s1 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e16);
        }
      }
      if (s1 !== peg$FAILED) {
        s2 = input.charAt(peg$currPos);
        if (peg$r8.test(s2)) {
          peg$currPos++;
        } else {
          s2 = peg$FAILED;
          if (peg$silentFails === 0) {
            peg$fail(peg$e17);
          }
        }
        if (s2 !== peg$FAILED) {
          s1 = [s1, s2];
          s0 = s1;
        } else {
          peg$currPos = s0;
          s0 = peg$FAILED;
        }
      } else {
        peg$currPos = s0;
        s0 = peg$FAILED;
      }
      return s0;
    }
    function peg$parsepawn_col() {
      var s0;
      s0 = input.charAt(peg$currPos);
      if (peg$r8.test(s0)) {
        peg$currPos++;
      } else {
        s0 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e17);
        }
      }
      return s0;
    }
    function peg$parseICCS_move() {
      var s0, s1, s2, s3, s4, s5, s6;
      s0 = peg$currPos;
      s1 = input.charAt(peg$currPos);
      if (peg$r10.test(s1)) {
        peg$currPos++;
      } else {
        s1 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e19);
        }
      }
      if (s1 !== peg$FAILED) {
        s2 = input.charAt(peg$currPos);
        if (peg$r2.test(s2)) {
          peg$currPos++;
        } else {
          s2 = peg$FAILED;
          if (peg$silentFails === 0) {
            peg$fail(peg$e9);
          }
        }
        if (s2 !== peg$FAILED) {
          if (input.charCodeAt(peg$currPos) === 45) {
            s3 = peg$c4;
            peg$currPos++;
          } else {
            s3 = peg$FAILED;
            if (peg$silentFails === 0) {
              peg$fail(peg$e20);
            }
          }
          if (s3 === peg$FAILED) {
            s3 = null;
          }
          s4 = input.charAt(peg$currPos);
          if (peg$r10.test(s4)) {
            peg$currPos++;
          } else {
            s4 = peg$FAILED;
            if (peg$silentFails === 0) {
              peg$fail(peg$e19);
            }
          }
          if (s4 !== peg$FAILED) {
            s5 = input.charAt(peg$currPos);
            if (peg$r2.test(s5)) {
              peg$currPos++;
            } else {
              s5 = peg$FAILED;
              if (peg$silentFails === 0) {
                peg$fail(peg$e9);
              }
            }
            if (s5 !== peg$FAILED) {
              s6 = input.charAt(peg$currPos);
              if (peg$r5.test(s6)) {
                peg$currPos++;
              } else {
                s6 = peg$FAILED;
                if (peg$silentFails === 0) {
                  peg$fail(peg$e14);
                }
              }
              if (s6 === peg$FAILED) {
                s6 = null;
              }
              s1 = [s1, s2, s3, s4, s5, s6];
              s0 = s1;
            } else {
              peg$currPos = s0;
              s0 = peg$FAILED;
            }
          } else {
            peg$currPos = s0;
            s0 = peg$FAILED;
          }
        } else {
          peg$currPos = s0;
          s0 = peg$FAILED;
        }
      } else {
        peg$currPos = s0;
        s0 = peg$FAILED;
      }
      return s0;
    }
    function peg$parsesuffixAnnotation() {
      var s0, s1, s2, s3;
      peg$silentFails++;
      s0 = peg$currPos;
      s1 = peg$currPos;
      s2 = [];
      s3 = input.charAt(peg$currPos);
      if (peg$r11.test(s3)) {
        peg$currPos++;
      } else {
        s3 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e22);
        }
      }
      while (s3 !== peg$FAILED) {
        s2.push(s3);
        if (s2.length >= 2) {
          s3 = peg$FAILED;
        } else {
          s3 = input.charAt(peg$currPos);
          if (peg$r11.test(s3)) {
            peg$currPos++;
          } else {
            s3 = peg$FAILED;
            if (peg$silentFails === 0) {
              peg$fail(peg$e22);
            }
          }
        }
      }
      if (s2.length < 1) {
        peg$currPos = s1;
        s1 = peg$FAILED;
      } else {
        s1 = s2;
      }
      if (s1 !== peg$FAILED) {
        s0 = input.substring(s0, peg$currPos);
      } else {
        s0 = s1;
      }
      peg$silentFails--;
      if (s0 === peg$FAILED) {
        s1 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e21);
        }
      }
      return s0;
    }
    function peg$parsenag() {
      var s0, s2, s3, s4, s5;
      peg$silentFails++;
      s0 = peg$currPos;
      peg$parse_();
      if (input.charCodeAt(peg$currPos) === 36) {
        s2 = peg$c5;
        peg$currPos++;
      } else {
        s2 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e24);
        }
      }
      if (s2 !== peg$FAILED) {
        s3 = peg$currPos;
        s4 = [];
        s5 = input.charAt(peg$currPos);
        if (peg$r2.test(s5)) {
          peg$currPos++;
        } else {
          s5 = peg$FAILED;
          if (peg$silentFails === 0) {
            peg$fail(peg$e9);
          }
        }
        if (s5 !== peg$FAILED) {
          while (s5 !== peg$FAILED) {
            s4.push(s5);
            s5 = input.charAt(peg$currPos);
            if (peg$r2.test(s5)) {
              peg$currPos++;
            } else {
              s5 = peg$FAILED;
              if (peg$silentFails === 0) {
                peg$fail(peg$e9);
              }
            }
          }
        } else {
          s4 = peg$FAILED;
        }
        if (s4 !== peg$FAILED) {
          s3 = input.substring(s3, peg$currPos);
        } else {
          s3 = s4;
        }
        if (s3 !== peg$FAILED) {
          s0 = peg$f6(s3);
        } else {
          peg$currPos = s0;
          s0 = peg$FAILED;
        }
      } else {
        peg$currPos = s0;
        s0 = peg$FAILED;
      }
      peg$silentFails--;
      if (s0 === peg$FAILED) {
        if (peg$silentFails === 0) {
          peg$fail(peg$e23);
        }
      }
      return s0;
    }
    function peg$parsecomment() {
      var s0;
      s0 = peg$parsebraceComment();
      if (s0 === peg$FAILED) {
        s0 = peg$parserestOfLineComment();
      }
      return s0;
    }
    function peg$parsebraceComment() {
      var s0, s1, s2, s3, s4;
      peg$silentFails++;
      s0 = peg$currPos;
      if (input.charCodeAt(peg$currPos) === 123) {
        s1 = peg$c6;
        peg$currPos++;
      } else {
        s1 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e26);
        }
      }
      if (s1 !== peg$FAILED) {
        s2 = peg$currPos;
        s3 = [];
        s4 = input.charAt(peg$currPos);
        if (peg$r12.test(s4)) {
          peg$currPos++;
        } else {
          s4 = peg$FAILED;
          if (peg$silentFails === 0) {
            peg$fail(peg$e27);
          }
        }
        while (s4 !== peg$FAILED) {
          s3.push(s4);
          s4 = input.charAt(peg$currPos);
          if (peg$r12.test(s4)) {
            peg$currPos++;
          } else {
            s4 = peg$FAILED;
            if (peg$silentFails === 0) {
              peg$fail(peg$e27);
            }
          }
        }
        s2 = input.substring(s2, peg$currPos);
        if (input.charCodeAt(peg$currPos) === 125) {
          s3 = peg$c7;
          peg$currPos++;
        } else {
          s3 = peg$FAILED;
          if (peg$silentFails === 0) {
            peg$fail(peg$e28);
          }
        }
        if (s3 !== peg$FAILED) {
          s0 = peg$f7(s2);
        } else {
          peg$currPos = s0;
          s0 = peg$FAILED;
        }
      } else {
        peg$currPos = s0;
        s0 = peg$FAILED;
      }
      peg$silentFails--;
      if (s0 === peg$FAILED) {
        s1 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e25);
        }
      }
      return s0;
    }
    function peg$parserestOfLineComment() {
      var s0, s1, s2, s3, s4;
      peg$silentFails++;
      s0 = peg$currPos;
      if (input.charCodeAt(peg$currPos) === 59) {
        s1 = peg$c8;
        peg$currPos++;
      } else {
        s1 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e30);
        }
      }
      if (s1 !== peg$FAILED) {
        s2 = peg$currPos;
        s3 = [];
        s4 = input.charAt(peg$currPos);
        if (peg$r13.test(s4)) {
          peg$currPos++;
        } else {
          s4 = peg$FAILED;
          if (peg$silentFails === 0) {
            peg$fail(peg$e31);
          }
        }
        while (s4 !== peg$FAILED) {
          s3.push(s4);
          s4 = input.charAt(peg$currPos);
          if (peg$r13.test(s4)) {
            peg$currPos++;
          } else {
            s4 = peg$FAILED;
            if (peg$silentFails === 0) {
              peg$fail(peg$e31);
            }
          }
        }
        s2 = input.substring(s2, peg$currPos);
        s0 = peg$f8(s2);
      } else {
        peg$currPos = s0;
        s0 = peg$FAILED;
      }
      peg$silentFails--;
      if (s0 === peg$FAILED) {
        s1 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e29);
        }
      }
      return s0;
    }
    function peg$parsevariation() {
      var s0, s2, s3, s5;
      peg$silentFails++;
      s0 = peg$currPos;
      peg$parse_();
      if (input.charCodeAt(peg$currPos) === 40) {
        s2 = peg$c9;
        peg$currPos++;
      } else {
        s2 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e33);
        }
      }
      if (s2 !== peg$FAILED) {
        s3 = peg$parseline();
        if (s3 !== peg$FAILED) {
          peg$parse_();
          if (input.charCodeAt(peg$currPos) === 41) {
            s5 = peg$c10;
            peg$currPos++;
          } else {
            s5 = peg$FAILED;
            if (peg$silentFails === 0) {
              peg$fail(peg$e34);
            }
          }
          if (s5 !== peg$FAILED) {
            s0 = peg$f9(s3);
          } else {
            peg$currPos = s0;
            s0 = peg$FAILED;
          }
        } else {
          peg$currPos = s0;
          s0 = peg$FAILED;
        }
      } else {
        peg$currPos = s0;
        s0 = peg$FAILED;
      }
      peg$silentFails--;
      if (s0 === peg$FAILED) {
        if (peg$silentFails === 0) {
          peg$fail(peg$e32);
        }
      }
      return s0;
    }
    function peg$parsegameTerminationMarker() {
      var s0, s1, s3;
      peg$silentFails++;
      s0 = peg$currPos;
      if (input.substr(peg$currPos, 3) === peg$c11) {
        s1 = peg$c11;
        peg$currPos += 3;
      } else {
        s1 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e36);
        }
      }
      if (s1 === peg$FAILED) {
        if (input.substr(peg$currPos, 3) === peg$c12) {
          s1 = peg$c12;
          peg$currPos += 3;
        } else {
          s1 = peg$FAILED;
          if (peg$silentFails === 0) {
            peg$fail(peg$e37);
          }
        }
        if (s1 === peg$FAILED) {
          if (input.substr(peg$currPos, 7) === peg$c13) {
            s1 = peg$c13;
            peg$currPos += 7;
          } else {
            s1 = peg$FAILED;
            if (peg$silentFails === 0) {
              peg$fail(peg$e38);
            }
          }
          if (s1 === peg$FAILED) {
            if (input.charCodeAt(peg$currPos) === 42) {
              s1 = peg$c14;
              peg$currPos++;
            } else {
              s1 = peg$FAILED;
              if (peg$silentFails === 0) {
                peg$fail(peg$e39);
              }
            }
          }
        }
      }
      if (s1 !== peg$FAILED) {
        peg$parse_();
        s3 = peg$parsecomment();
        if (s3 === peg$FAILED) {
          s3 = null;
        }
        s0 = peg$f10(s1, s3);
      } else {
        peg$currPos = s0;
        s0 = peg$FAILED;
      }
      peg$silentFails--;
      if (s0 === peg$FAILED) {
        s1 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e35);
        }
      }
      return s0;
    }
    function peg$parse_() {
      var s0, s1;
      peg$silentFails++;
      s0 = [];
      s1 = input.charAt(peg$currPos);
      if (peg$r14.test(s1)) {
        peg$currPos++;
      } else {
        s1 = peg$FAILED;
        if (peg$silentFails === 0) {
          peg$fail(peg$e41);
        }
      }
      while (s1 !== peg$FAILED) {
        s0.push(s1);
        s1 = input.charAt(peg$currPos);
        if (peg$r14.test(s1)) {
          peg$currPos++;
        } else {
          s1 = peg$FAILED;
          if (peg$silentFails === 0) {
            peg$fail(peg$e41);
          }
        }
      }
      peg$silentFails--;
      s1 = peg$FAILED;
      if (peg$silentFails === 0) {
        peg$fail(peg$e40);
      }
      return s0;
    }
    peg$result = peg$startRuleFunction();
    if (options.peg$library) {
      return (
        /** @type {any} */
        {
          peg$result,
          peg$currPos,
          peg$FAILED,
          peg$maxFailExpected,
          peg$maxFailPos
        }
      );
    }
    if (peg$result !== peg$FAILED && peg$currPos === input.length) {
      return peg$result;
    } else {
      if (peg$result !== peg$FAILED && peg$currPos < input.length) {
        peg$fail(peg$endExpectation());
      }
      throw peg$buildStructuredError(
        peg$maxFailExpected,
        peg$maxFailPos < input.length ? input.charAt(peg$maxFailPos) : null,
        peg$maxFailPos < input.length ? peg$computeLocation(peg$maxFailPos, peg$maxFailPos + 1) : peg$computeLocation(peg$maxFailPos, peg$maxFailPos)
      );
    }
  }
  var MASK64 = 0xffffffffffffffffn;
  var WHITE = "w";
  var BLACK = "b";
  var KING = "k";
  var ADVISOR = "a";
  var ELEPHANT = "b";
  var HORSE = "n";
  var ROOK = "r";
  var CANNON = "c";
  var PAWN = "p";
  var SYMBOLS = "pnrbckaPNRBCKA";
  var EMPTY = -1;
  var FLAGS = {
    NORMAL: "n",
    CAPTURE: "c",
    NULL_MOVE: "-"
  };
  var BITS = {
    NORMAL: 1,
    CAPTURE: 2,
    NULL_MOVE: 128
  };
  var SAN_NULLMOVE = "--";
  var DEFAULT_POSITION = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1";
  var XQ_SQUARES = {
    a9: 144,
    b9: 145,
    c9: 146,
    d9: 147,
    e9: 148,
    f9: 149,
    g9: 150,
    h9: 151,
    i9: 152,
    a8: 128,
    b8: 129,
    c8: 130,
    d8: 131,
    e8: 132,
    f8: 133,
    g8: 134,
    h8: 135,
    i8: 136,
    a7: 112,
    b7: 113,
    c7: 114,
    d7: 115,
    e7: 116,
    f7: 117,
    g7: 118,
    h7: 119,
    i7: 120,
    a6: 96,
    b6: 97,
    c6: 98,
    d6: 99,
    e6: 100,
    f6: 101,
    g6: 102,
    h6: 103,
    i6: 104,
    a5: 80,
    b5: 81,
    c5: 82,
    d5: 83,
    e5: 84,
    f5: 85,
    g5: 86,
    h5: 87,
    i5: 88,
    a4: 64,
    b4: 65,
    c4: 66,
    d4: 67,
    e4: 68,
    f4: 69,
    g4: 70,
    h4: 71,
    i4: 72,
    a3: 48,
    b3: 49,
    c3: 50,
    d3: 51,
    e3: 52,
    f3: 53,
    g3: 54,
    h3: 55,
    i3: 56,
    a2: 32,
    b2: 33,
    c2: 34,
    d2: 35,
    e2: 36,
    f2: 37,
    g2: 38,
    h2: 39,
    i2: 40,
    a1: 16,
    b1: 17,
    c1: 18,
    d1: 19,
    e1: 20,
    f1: 21,
    g1: 22,
    h1: 23,
    i1: 24,
    a0: 0,
    b0: 1,
    c0: 2,
    d0: 3,
    e0: 4,
    f0: 5,
    g0: 6,
    h0: 7,
    i0: 8
  };
  var PIECE_OFFSETS = {
    k: [-16, 1, 16, -1],
    a: [-17, -15, 15, 17],
    b: [-34, -30, 30, 34],
    n: [-33, -31, -18, -14, 14, 18, 31, 33],
    r: [-16, 1, 16, -1],
    c: [-16, 1, 16, -1],
    p: []
  };
  var HORSE_LEGS = {
    [-33]: -16,
    [-31]: -16,
    [-18]: -1,
    [-14]: 1,
    [14]: -1,
    [18]: 1,
    [31]: 16,
    [33]: 16
  };
  var ELEPHANT_EYES = {
    [-34]: -17,
    [-30]: -15,
    [30]: 15,
    [34]: 17
  };
  var WXF_LETTER = {
    k: "K",
    a: "A",
    b: "B",
    n: "N",
    r: "R",
    c: "C",
    p: "P"
  };
  var SUFFIX_LIST = ["!", "?", "!!", "!?", "?!", "??"];
  var SEVEN_TAG_ROSTER = {
    Event: "?",
    Site: "?",
    Date: "????.??.??",
    Round: "?",
    White: "?",
    Black: "?",
    Result: "*"
  };
  var SUPPLEMENTAL_TAGS = {
    WhiteTitle: null,
    BlackTitle: null,
    WhiteElo: null,
    BlackElo: null,
    WhiteUSCF: null,
    BlackUSCF: null,
    WhiteNA: null,
    BlackNA: null,
    WhiteType: null,
    BlackType: null,
    EventDate: null,
    EventSponsor: null,
    Section: null,
    Stage: null,
    Board: null,
    Opening: null,
    Variation: null,
    SubVariation: null,
    ECO: null,
    NIC: null,
    Time: null,
    UTCTime: null,
    UTCDate: null,
    TimeControl: null,
    SetUp: null,
    FEN: null,
    Termination: null,
    Annotator: null,
    Mode: null,
    PlyCount: null
  };
  var HEADER_TEMPLATE = {
    ...SEVEN_TAG_ROSTER,
    ...SUPPLEMENTAL_TAGS
  };
  function offBoard(square) {
    return square < 0 || (square & 15) >= 9 || square >> 4 >= 10;
  }
  function rank(square) {
    return square >> 4;
  }
  function file(square) {
    return square & 15;
  }
  function isDigit(c) {
    return "0123456789".indexOf(c) !== -1;
  }
  function algebraic(square) {
    const f = file(square);
    const r = rank(square);
    return String.fromCharCode(97 + f) + r;
  }
  function swapColor(color) {
    return color === WHITE ? BLACK : WHITE;
  }
  function inPalace(color, r, f) {
    if (f < 3 || f > 5)
      return false;
    if (color === WHITE)
      return r >= 0 && r <= 2;
    return r >= 7 && r <= 9;
  }
  function crossedRiver(color, r) {
    if (color === WHITE)
      return r >= 5;
    return r <= 4;
  }
  var Move = class {
    color;
    from;
    to;
    piece;
    captured;
    flags;
    zh;
    lan;
    before;
    after;
    wxf;
    iccs;
    isCheck;
    isCheckmate;
    constructor(internal, zh, before, after, wxf, iccs, isCheck = false, isCheckmate = false) {
      const { color, piece, from, to, flags, captured } = internal;
      this.color = color;
      this.piece = piece;
      this.from = algebraic(from);
      this.to = algebraic(to);
      this.zh = zh;
      this.lan = algebraic(from) + algebraic(to);
      this.before = before;
      this.after = after;
      this.wxf = wxf;
      this.iccs = iccs;
      this.isCheck = isCheck;
      this.isCheckmate = isCheckmate;
      this.flags = "";
      for (const flag in BITS) {
        if (BITS[flag] & flags) {
          this.flags += FLAGS[flag];
        }
      }
      if (captured) {
        this.captured = captured;
      }
    }
    isCapture() {
      return this.flags.indexOf(FLAGS["CAPTURE"]) > -1;
    }
    isNullMove() {
      return this.flags.indexOf(FLAGS["NULL_MOVE"]) > -1;
    }
  };
  function rotl(x, k) {
    return (x << k | x >> 64n - k) & 0xffffffffffffffffn;
  }
  function wrappingMul(x, y) {
    return x * y & MASK64;
  }
  function xoroshiro128(state) {
    return function() {
      let s0 = BigInt(state & MASK64);
      let s1 = BigInt(state >> 64n & MASK64);
      const result = wrappingMul(rotl(wrappingMul(s0, 5n), 7n), 9n);
      s1 ^= s0;
      s0 = (rotl(s0, 24n) ^ s1 ^ s1 << 16n) & MASK64;
      s1 = rotl(s1, 37n);
      state = s1 << 64n | s0;
      return result;
    };
  }
  var rand = xoroshiro128(0xa187eb39cdcaed8f31c4b365b102e01en);
  var PIECE_KEYS = Array.from({ length: 2 }, () => Array.from({ length: 7 }, () => Array.from({ length: 160 }, () => rand())));
  var SIDE_KEY = rand();
  function pieceKey(board, i) {
    if (!board[i])
      return 0n;
    const { color, type } = board[i];
    const colorIndex = {
      w: 0,
      b: 1
    }[color];
    const typeIndex = {
      p: 0,
      n: 1,
      b: 2,
      r: 3,
      c: 4,
      k: 5,
      a: 6
    }[type];
    return PIECE_KEYS[colorIndex][typeIndex][i];
  }
  function computeHash(board, turn) {
    let hash = 0n;
    for (let i = XQ_SQUARES.a0; i <= XQ_SQUARES.i9; i++) {
      if ((i & 15) >= 9) {
        i += 6;
        continue;
      }
      if (board[i]) {
        hash ^= pieceKey(board, i);
      }
    }
    if (turn === "b") {
      hash ^= SIDE_KEY;
    }
    return hash;
  }
  function validateFen(fen) {
    const tokens = fen.trim().split(/\s+/);
    if (tokens.length < 1 || tokens.length > 6) {
      return {
        ok: false,
        error: "Invalid FEN: must contain 1 to 6 space-delimited fields"
      };
    }
    if (tokens.length >= 6) {
      const moveNumber = parseInt(tokens[5], 10);
      if (isNaN(moveNumber) || moveNumber <= 0) {
        return {
          ok: false,
          error: "Invalid FEN: move number must be a positive integer"
        };
      }
    }
    if (tokens.length >= 5) {
      const halfMoves = parseInt(tokens[4], 10);
      if (isNaN(halfMoves) || halfMoves < 0) {
        return {
          ok: false,
          error: "Invalid FEN: half move counter must be a non-negative integer"
        };
      }
    }
    if (tokens.length >= 4) {
      if (tokens[3] !== "-") {
        return { ok: false, error: 'Invalid FEN: en-passant must be "-"' };
      }
    }
    if (tokens.length >= 3) {
      if (tokens[2] !== "-") {
        return {
          ok: false,
          error: 'Invalid FEN: castling must be "-"'
        };
      }
    }
    if (tokens.length >= 2) {
      if (!/^(w|b)$/.test(tokens[1])) {
        return { ok: false, error: "Invalid FEN: side-to-move is invalid" };
      }
    }
    const rows = tokens[0].split("/");
    if (rows.length !== 10) {
      return {
        ok: false,
        error: "Invalid FEN: piece data does not contain 10 '/'-delimited rows"
      };
    }
    for (let i = 0; i < rows.length; i++) {
      let sumFields = 0;
      let previousWasNumber = false;
      for (let k = 0; k < rows[i].length; k++) {
        if (isDigit(rows[i][k])) {
          if (previousWasNumber) {
            return {
              ok: false,
              error: "Invalid FEN: piece data is invalid (consecutive number)"
            };
          }
          sumFields += parseInt(rows[i][k], 10);
          previousWasNumber = true;
        } else {
          if (!/^[pnrbckahePNRBCKAHE]$/.test(rows[i][k])) {
            return {
              ok: false,
              error: "Invalid FEN: piece data is invalid (invalid piece)"
            };
          }
          sumFields += 1;
          previousWasNumber = false;
        }
      }
      if (sumFields !== 9) {
        return {
          ok: false,
          error: "Invalid FEN: piece data is invalid (too many squares in rank)"
        };
      }
    }
    const kings = [
      { color: "red", regex: /K/g },
      { color: "black", regex: /k/g }
    ];
    for (const { color, regex } of kings) {
      if (!regex.test(tokens[0])) {
        return { ok: false, error: `Invalid FEN: missing ${color} king` };
      }
      if ((tokens[0].match(regex) || []).length > 1) {
        return { ok: false, error: `Invalid FEN: too many ${color} kings` };
      }
    }
    const board = [];
    for (let i = 0; i < rows.length; i++) {
      const row = [];
      for (let k = 0; k < rows[i].length; k++) {
        if (isDigit(rows[i][k])) {
          for (let d = 0; d < parseInt(rows[i][k], 10); d++) {
            row.push(null);
          }
        } else {
          row.push(rows[i][k]);
        }
      }
      board.push(row);
    }
    const RED_ADVISOR_POSITIONS = /* @__PURE__ */ new Set(["3,9", "5,9", "4,8", "3,7", "5,7"]);
    const BLACK_ADVISOR_POSITIONS = /* @__PURE__ */ new Set(["3,0", "5,0", "4,1", "3,2", "5,2"]);
    for (let r = 0; r < 10; r++) {
      for (let f = 0; f < 9; f++) {
        const piece = board[r][f];
        if (piece === "A") {
          if (!RED_ADVISOR_POSITIONS.has(`${f},${r}`)) {
            return {
              ok: false,
              error: "Invalid FEN: red advisor is on an invalid position"
            };
          }
        } else if (piece === "a") {
          if (!BLACK_ADVISOR_POSITIONS.has(`${f},${r}`)) {
            return {
              ok: false,
              error: "Invalid FEN: black advisor is on an invalid position"
            };
          }
        }
      }
    }
    const RED_ELEPHANT_POSITIONS = /* @__PURE__ */ new Set([
      "2,9",
      "6,9",
      "0,7",
      "4,7",
      "8,7",
      "2,5",
      "6,5"
    ]);
    const BLACK_ELEPHANT_POSITIONS = /* @__PURE__ */ new Set([
      "2,0",
      "6,0",
      "0,2",
      "4,2",
      "8,2",
      "2,4",
      "6,4"
    ]);
    for (let r = 0; r < 10; r++) {
      for (let f = 0; f < 9; f++) {
        const piece = board[r][f];
        if (piece === "B" || piece === "E") {
          if (!RED_ELEPHANT_POSITIONS.has(`${f},${r}`)) {
            return {
              ok: false,
              error: "Invalid FEN: red elephant is on an invalid position"
            };
          }
        } else if (piece === "b" || piece === "e") {
          if (!BLACK_ELEPHANT_POSITIONS.has(`${f},${r}`)) {
            return {
              ok: false,
              error: "Invalid FEN: black elephant is on an invalid position"
            };
          }
        }
      }
    }
    let redKingRow = -1;
    let redKingFile = -1;
    let blackKingRow = -1;
    let blackKingFile = -1;
    for (let r = 0; r < 10; r++) {
      for (let f = 0; f < 9; f++) {
        if (board[r][f] === "K") {
          redKingRow = r;
          redKingFile = f;
        } else if (board[r][f] === "k") {
          blackKingRow = r;
          blackKingFile = f;
        }
      }
    }
    if (redKingFile === blackKingFile) {
      let blocked = false;
      const minRow = Math.min(redKingRow, blackKingRow);
      const maxRow = Math.max(redKingRow, blackKingRow);
      for (let r = minRow + 1; r < maxRow; r++) {
        if (board[r][redKingFile] !== null) {
          blocked = true;
          break;
        }
      }
      if (!blocked) {
        return {
          ok: false,
          error: "Invalid FEN: kings are facing each other (flying general)"
        };
      }
    }
    return { ok: true };
  }
  function parseFen(fen) {
    let tokens = fen.trim().split(/\s+/);
    if (tokens.length >= 1 && tokens.length < 6) {
      const adjustments = ["w", "-", "-", "0", "1"];
      fen = tokens.concat(adjustments.slice(-(6 - tokens.length))).join(" ");
    }
    tokens = fen.split(/\s+/);
    const position = tokens[0];
    let rank2 = 9;
    let file2 = 0;
    const board = new Array(160);
    for (let i = 0; i < position.length; i++) {
      const piece = position.charAt(i);
      if (piece === "/") {
        rank2--;
        file2 = 0;
      } else if (isDigit(piece)) {
        file2 += parseInt(piece, 10);
      } else {
        const color = piece < "a" ? WHITE : BLACK;
        let type = piece.toLowerCase();
        if (type === "e")
          type = "b";
        else if (type === "h")
          type = "n";
        board[rank2 * 16 + file2] = {
          type,
          color
        };
        file2++;
      }
    }
    return {
      board,
      turn: tokens[1],
      halfMoves: parseInt(tokens[4], 10),
      moveNumber: parseInt(tokens[5], 10)
    };
  }
  function serializeFen(board, turn, halfMoves, moveNumber) {
    let fen = "";
    for (let r = 9; r >= 0; r--) {
      let empty = 0;
      for (let f = 0; f < 9; f++) {
        const i = r * 16 + f;
        if (board[i]) {
          if (empty > 0) {
            fen += empty;
            empty = 0;
          }
          const { color, type } = board[i];
          fen += color === WHITE ? type.toUpperCase() : type.toLowerCase();
        } else {
          empty++;
        }
      }
      if (empty > 0) {
        fen += empty;
      }
      if (r > 0) {
        fen += "/";
      }
    }
    return [fen, turn, "-", "-", halfMoves, moveNumber].join(" ");
  }
  function inferPieceType(san) {
    let pieceType = san.charAt(0);
    if (pieceType >= "a" && pieceType <= "i") {
      const matches = san.match(/[a-i]\d.*[a-i]\d/);
      if (matches) {
        return void 0;
      }
      return PAWN;
    }
    pieceType = pieceType.toLowerCase();
    if (pieceType === "k")
      return KING;
    if (pieceType === "a")
      return ADVISOR;
    if (pieceType === "b" || pieceType === "e")
      return ELEPHANT;
    if (pieceType === "n" || pieceType === "h")
      return HORSE;
    if (pieceType === "r")
      return ROOK;
    if (pieceType === "c")
      return CANNON;
    return pieceType;
  }
  function strippedSan(move) {
    return move.replace(/[+#]?[?!]*$/, "");
  }
  function moveToWxf(board, move) {
    if (move.flags & BITS.NULL_MOVE) {
      return "--";
    }
    const { piece, color, from, to } = move;
    const isRed = color === WHITE;
    const letter = WXF_LETTER[piece];
    const pieceLetter = isRed ? letter : letter.toLowerCase();
    const labels = ["a", "b", "c", "d", "e"];
    const numbers = ["1", "2", "3", "4", "5", "6", "7", "8", "9"];
    const BOARD = Array.from({ length: 9 }, () => Array(10).fill(null));
    let fromX, fromY, toX, toY;
    if (isRed) {
      for (let x = 0; x < 9; x++) {
        for (let y = 0; y < 10; y++) {
          BOARD[x][y] = board[(9 - y) * 16 + (8 - x)];
        }
      }
      fromX = 8 - file(from);
      fromY = 9 - rank(from);
      toX = 8 - file(to);
      toY = 9 - rank(to);
    } else {
      for (let x = 0; x < 9; x++) {
        for (let y = 0; y < 10; y++) {
          BOARD[x][y] = board[y * 16 + x];
        }
      }
      fromX = file(from);
      fromY = rank(from);
      toX = file(to);
      toY = rank(to);
    }
    let prefix;
    if (piece === "a" || piece === "b") {
      prefix = pieceLetter + numbers[fromX];
    } else if (piece === "p") {
      prefix = wxfPawnPrefix(piece, color, pieceLetter, labels, numbers, BOARD, fromX, fromY);
    } else {
      const sameCol = [];
      for (let y = 0; y < 10; y++) {
        const p = BOARD[fromX][y];
        if (p && p.type === piece && p.color === color) {
          sameCol.push(y);
        }
      }
      if (sameCol.length <= 1) {
        prefix = pieceLetter + numbers[fromX];
      } else {
        sameCol.sort((a, b) => a - b);
        const idx = sameCol.indexOf(fromY);
        if (sameCol.length === 2) {
          prefix = (idx === 0 ? "+" : "-") + pieceLetter;
        } else if (sameCol.length === 3) {
          prefix = (idx === 0 ? "+" : idx === 1 ? "." : "-") + pieceLetter;
        } else {
          prefix = pieceLetter + labels[idx];
        }
      }
    }
    let action, target;
    if (fromY === toY) {
      action = ".";
      target = numbers[toX];
    } else {
      const forward = toY < fromY;
      action = forward ? "+" : "-";
      if (piece === "r" || piece === "c" || piece === "k" || piece === "p") {
        const steps = Math.abs(toY - fromY);
        target = numbers[steps - 1];
      } else {
        target = numbers[toX];
      }
    }
    return prefix + action + target;
  }
  function wxfPawnPrefix(piece, color, pieceLetter, labels, numbers, board2d, fromX, fromY) {
    const colMap = /* @__PURE__ */ new Map();
    for (let x = 0; x < 9; x++) {
      for (let y = 0; y < 10; y++) {
        const p = board2d[x][y];
        if (p && p.type === piece && p.color === color) {
          if (!colMap.has(x))
            colMap.set(x, []);
          colMap.get(x).push(y);
        }
      }
    }
    for (const rows2 of colMap.values()) {
      rows2.sort((a, b) => a - b);
    }
    const multiCols = [...colMap.entries()].filter(([, rows2]) => rows2.length > 1).map(([x]) => x);
    if (!multiCols.includes(fromX)) {
      return pieceLetter + numbers[fromX];
    }
    const rows = colMap.get(fromX);
    const idx = rows.indexOf(fromY);
    if (multiCols.length === 1) {
      if (rows.length === 2)
        return (idx === 0 ? "+" : "-") + pieceLetter;
      if (rows.length === 3) {
        return (idx === 0 ? "+" : idx === 1 ? "." : "-") + pieceLetter;
      }
      return pieceLetter + labels[idx];
    }
    const sortedCols = [...multiCols].sort((a, b) => a - b);
    const allPawns = [];
    for (const x of sortedCols) {
      for (const y of colMap.get(x)) {
        allPawns.push({ x, y });
      }
    }
    const globalIdx = allPawns.findIndex((p) => p.x === fromX && p.y === fromY);
    return pieceLetter + labels[globalIdx];
  }
  function moveToZh(wxf, color) {
    if (wxf === "--")
      return "--";
    const isRed = color === WHITE;
    const redNums = ["\u4E00", "\u4E8C", "\u4E09", "\u56DB", "\u4E94", "\u516D", "\u4E03", "\u516B", "\u4E5D"];
    const PIECE_REV = {
      K: "\u5E05",
      k: "\u5C06",
      A: "\u4ED5",
      a: "\u58EB",
      B: "\u76F8",
      b: "\u8C61",
      N: "\u9A6C",
      n: "\u9A6C",
      R: "\u8F66",
      r: "\u8F66",
      C: "\u70AE",
      c: "\u70AE",
      P: "\u5175",
      p: "\u5352"
    };
    const re = /^([+\-.a-e])?([A-Za-z])([a-e])?(\d)?([+\-.])(\d)$/;
    const m = wxf.match(re);
    if (!m)
      return wxf;
    const prefix = m[1] || "";
    const pieceKey2 = m[2];
    const label = m[3] || "";
    const col = m[4] || "";
    const action = m[5];
    const target = m[6];
    const pieceChar = PIECE_REV[pieceKey2];
    if (!pieceChar)
      return wxf;
    let zhPrefix = "";
    if (prefix === "+")
      zhPrefix = "\u524D";
    else if (prefix === ".")
      zhPrefix = "\u4E2D";
    else if (prefix === "-")
      zhPrefix = "\u540E";
    else if (prefix === "a")
      zhPrefix = "\u4E00";
    else if (prefix === "b")
      zhPrefix = "\u4E8C";
    else if (prefix === "c")
      zhPrefix = "\u4E09";
    else if (prefix === "d")
      zhPrefix = "\u56DB";
    else if (prefix === "e")
      zhPrefix = "\u4E94";
    if (!zhPrefix && label) {
      zhPrefix = { "a": "\u4E00", "b": "\u4E8C", "c": "\u4E09", "d": "\u56DB", "e": "\u4E94" }[label] ?? "";
    }
    let zhAction;
    if (action === ".")
      zhAction = "\u5E73";
    else if (action === "+")
      zhAction = "\u8FDB";
    else
      zhAction = "\u9000";
    const toZhNum = (n) => isRed ? redNums[parseInt(n) - 1] : "\uFF11\uFF12\uFF13\uFF14\uFF15\uFF16\uFF17\uFF18\uFF19"[parseInt(n) - 1];
    let zh;
    if (zhPrefix) {
      zh = zhPrefix + pieceChar + zhAction + toZhNum(target);
    } else {
      zh = pieceChar + toZhNum(col) + zhAction + toZhNum(target);
    }
    return zh;
  }
  function moveToLan(move) {
    if (move.flags & BITS.NULL_MOVE) {
      return SAN_NULLMOVE;
    }
    return algebraic(move.from) + algebraic(move.to);
  }
  function moveToIccs(move) {
    if (move.flags & BITS.NULL_MOVE) {
      return SAN_NULLMOVE;
    }
    const fromSq = algebraic(move.from);
    const toSq = algebraic(move.to);
    return fromSq.toUpperCase() + "-" + toSq.toUpperCase();
  }
  function zhToWxf(zh) {
    const PIECE_MAP = {
      "\u5E05": "K",
      "\u5C06": "K",
      "\u4ED5": "A",
      "\u58EB": "A",
      "\u76F8": "B",
      "\u8C61": "B",
      "\u9A6C": "N",
      "\u99AC": "N",
      "\u508C": "N",
      "\u8F66": "R",
      "\u8ECA": "R",
      "\u4FE5": "R",
      "\u70AE": "C",
      "\u7832": "C",
      "\u5175": "P",
      "\u5352": "P"
    };
    const NUM_MAP = {
      "\u4E00": "1",
      "\u4E8C": "2",
      "\u4E09": "3",
      "\u56DB": "4",
      "\u4E94": "5",
      "\u516D": "6",
      "\u4E03": "7",
      "\u516B": "8",
      "\u4E5D": "9",
      "\uFF11": "1",
      "\uFF12": "2",
      "\uFF13": "3",
      "\uFF14": "4",
      "\uFF15": "5",
      "\uFF16": "6",
      "\uFF17": "7",
      "\uFF18": "8",
      "\uFF19": "9"
    };
    const ACTION_MAP = {
      "\u8FDB": "+",
      "\u9000": "-",
      "\u5E73": "."
    };
    const PREFIX_MAP = {
      "\u524D": "+",
      "\u4E2D": ".",
      "\u540E": "-"
    };
    const PIECE_CLS = "[\u8F66\u4FE5\u8ECA\u9A6C\u508C\u99AC\u70AE\u7832\u58EB\u4ED5\u8C61\u76F8\u5E05\u5C07\u5E05\u5C06\u5175\u5352]";
    const NUM_CLS = "[\u4E00\u4E8C\u4E09\u56DB\u4E94\u516D\u4E03\u516B\u4E5D\uFF11-\uFF191-9]";
    const ACT_CLS = "[\u8FDB\u9000\u5E73]";
    let m = zh.match(new RegExp(`^([\u524D\u540E\u4E2D])(${PIECE_CLS})(${ACT_CLS})(${NUM_CLS})$`));
    if (m) {
      return PIECE_MAP[m[2]] + PREFIX_MAP[m[1]] + ACTION_MAP[m[3]] + (NUM_MAP[m[4]] || m[4]);
    }
    m = zh.match(new RegExp(`^(${PIECE_CLS})(${NUM_CLS})(${ACT_CLS})(${NUM_CLS})$`));
    if (m) {
      return PIECE_MAP[m[1]] + (NUM_MAP[m[2]] || m[2]) + ACTION_MAP[m[3]] + (NUM_MAP[m[4]] || m[4]);
    }
    m = zh.match(new RegExp(`^([\u4E00\u4E8C\u4E09\u56DB\u4E94])(${PIECE_CLS})(${ACT_CLS})(${NUM_CLS})$`));
    if (m) {
      const label = { "\u4E00": "a", "\u4E8C": "b", "\u4E09": "c", "\u56DB": "d", "\u4E94": "e" }[m[1]];
      return PIECE_MAP[m[2]] + label + ACTION_MAP[m[3]] + (NUM_MAP[m[4]] || m[4]);
    }
    return "";
  }
  function moveFromSan(san, turn, legalMoves, strict = false) {
    const cleanMove = strippedSan(san);
    if (/[\u4e00-\u9fff]/.test(cleanMove)) {
      const letterFormat = zhToWxf(cleanMove);
      if (letterFormat)
        return moveFromSan(letterFormat, turn, legalMoves, strict);
      return null;
    }
    if (cleanMove == SAN_NULLMOVE) {
      return {
        color: turn,
        from: 0,
        to: 0,
        piece: "k",
        flags: BITS.NULL_MOVE
      };
    }
    const pieceType = inferPieceType(cleanMove);
    const candidateMoves = pieceType ? legalMoves.filter((m) => m.piece === pieceType) : legalMoves;
    for (let i = 0; i < candidateMoves.length; i++) {
      if (cleanMove === strippedSan(moveToLan(candidateMoves[i]))) {
        return candidateMoves[i];
      }
    }
    if (strict) {
      return null;
    }
    const iccsMatch = cleanMove.match(/^([a-i])([0-9])-?([a-i])([0-9])$/i);
    if (iccsMatch) {
      const fromSq = iccsMatch[1].toLowerCase() + iccsMatch[2];
      const toSq = iccsMatch[3].toLowerCase() + iccsMatch[4];
      if (!(fromSq in XQ_SQUARES) || !(toSq in XQ_SQUARES))
        return null;
      const fromIdx = XQ_SQUARES[fromSq];
      const toIdx = XQ_SQUARES[toSq];
      for (let i = 0; i < legalMoves.length; i++) {
        if (legalMoves[i].from === fromIdx && legalMoves[i].to === toIdx) {
          return legalMoves[i];
        }
      }
    }
    const labelMatch = cleanMove.match(/^([KABNRCPkabnrcp])([a-e])([+.x-])([0-9])$/);
    if (labelMatch) {
      const pieceLetter = labelMatch[1];
      const label = labelMatch[2];
      const action = labelMatch[3];
      const targetNum = parseInt(labelMatch[4], 10);
      const piece = pieceLetter.toLowerCase();
      const labelMoves = legalMoves.filter((m) => m.piece === piece);
      if (labelMoves.length < 4)
        return null;
      const isRed = turn === WHITE;
      labelMoves.sort((a, b) => {
        return isRed ? rank(b.from) - rank(a.from) : rank(a.from) - rank(b.from);
      });
      const seenFrom = /* @__PURE__ */ new Set();
      const sortedFroms = [];
      for (const m of labelMoves) {
        if (!seenFrom.has(m.from)) {
          seenFrom.add(m.from);
          sortedFroms.push(m.from);
        }
      }
      const labelIdx = label.charCodeAt(0) - 97;
      if (labelIdx < 0 || labelIdx >= sortedFroms.length)
        return null;
      const targetFrom = sortedFroms[labelIdx];
      for (const m of labelMoves) {
        if (m.from !== targetFrom)
          continue;
        if (action === ".") {
          let targetFile;
          if (turn === WHITE)
            targetFile = 9 - targetNum;
          else
            targetFile = targetNum - 1;
          if (file(m.to) === targetFile && rank(m.from) === rank(m.to))
            return m;
        } else if (action === "+" || action === "-") {
          if (piece === "r" || piece === "c" || piece === "k" || piece === "p") {
            const forward = turn === WHITE ? 1 : -1;
            if ((rank(m.to) - rank(m.from)) * forward === targetNum)
              return m;
          } else {
            let targetFile;
            if (turn === WHITE)
              targetFile = 9 - targetNum;
            else
              targetFile = targetNum - 1;
            if (file(m.to) === targetFile)
              return m;
          }
        }
      }
    }
    const fbMatch = cleanMove.match(/^([KABNRCPkabnrcp])([+\-.])([+.x-])([0-9])$/);
    if (fbMatch) {
      const pieceLetter = fbMatch[1];
      const prefix = fbMatch[2];
      const action = fbMatch[3];
      const targetNum = parseInt(fbMatch[4], 10);
      const piece = pieceLetter.toLowerCase();
      const byFile = /* @__PURE__ */ new Map();
      for (const m of legalMoves) {
        if (m.piece === piece) {
          const f = file(m.from);
          if (!byFile.has(f))
            byFile.set(f, []);
          byFile.get(f).push(m);
        }
      }
      for (const [, moves] of byFile) {
        if (moves.length < 2)
          continue;
        const isRed = turn === WHITE;
        moves.sort((a, b) => isRed ? rank(b.from) - rank(a.from) : rank(a.from) - rank(b.from));
        const seen = /* @__PURE__ */ new Set();
        const sorted = [];
        for (const m of moves) {
          if (!seen.has(m.from)) {
            seen.add(m.from);
            sorted.push(m.from);
          }
        }
        const idx = prefix === "+" ? 0 : prefix === "-" ? sorted.length - 1 : 1;
        if (idx < 0 || idx >= sorted.length)
          continue;
        const targetFrom = sorted[idx];
        for (const m of moves) {
          if (m.from !== targetFrom)
            continue;
          if (action === ".") {
            let targetFile;
            if (turn === WHITE)
              targetFile = 9 - targetNum;
            else
              targetFile = targetNum - 1;
            if (file(m.to) === targetFile && rank(m.from) === rank(m.to))
              return m;
          } else if (action === "+" || action === "-") {
            if (piece === "r" || piece === "c" || piece === "k" || piece === "p") {
              const forward = turn === WHITE ? 1 : -1;
              if ((rank(m.to) - rank(m.from)) * forward === targetNum)
                return m;
            } else {
              let targetFile;
              if (turn === WHITE)
                targetFile = 9 - targetNum;
              else
                targetFile = targetNum - 1;
              if (file(m.to) === targetFile)
                return m;
            }
          }
        }
      }
    }
    const wxfMatch = cleanMove.match(/^([KABNRCPkabnrcp])?([1-9])([+.x-])([0-9])/);
    if (wxfMatch) {
      const piece = wxfMatch[1]?.toLowerCase() || PAWN;
      const fromCol = parseInt(wxfMatch[2], 10);
      const action = wxfMatch[3];
      const targetNum = parseInt(wxfMatch[4], 10);
      let fromFile;
      if (turn === WHITE) {
        fromFile = 9 - fromCol;
      } else {
        fromFile = fromCol - 1;
      }
      const wxfMoves = legalMoves.filter((m) => m.piece === piece);
      for (let i = 0; i < wxfMoves.length; i++) {
        const m = wxfMoves[i];
        if (file(m.from) === fromFile) {
          const mToFile = file(m.to);
          const mToRank = rank(m.to);
          if (action === ".") {
            let targetFile;
            if (turn === WHITE) {
              targetFile = 9 - targetNum;
            } else {
              targetFile = targetNum - 1;
            }
            if (mToFile === targetFile && rank(m.from) === mToRank) {
              return m;
            }
          } else if (action === "+" || action === "-") {
            if (piece === "r" || piece === "c" || piece === "k" || piece === "p") {
              const forward = turn === WHITE ? 1 : -1;
              const actualSteps = (rank(m.to) - rank(m.from)) * forward;
              if (actualSteps === targetNum) {
                return m;
              }
            } else {
              let targetFile;
              if (turn === WHITE) {
                targetFile = 9 - targetNum;
              } else {
                targetFile = targetNum - 1;
              }
              if (mToFile === targetFile) {
                return m;
              }
            }
          }
        }
      }
    }
    return null;
  }
  function _pieceAttacksSquare(board, piece, color, from, to) {
    const targetFile = file(to);
    const targetRank = rank(to);
    const fromFile = file(from);
    const fromRank = rank(from);
    const df = targetFile - fromFile;
    const dr = targetRank - fromRank;
    const absDf = Math.abs(df);
    const absDr = Math.abs(dr);
    switch (piece.type) {
      case KING:
        if (inPalace(color, targetRank, targetFile) && (absDf === 1 && dr === 0 || df === 0 && absDr === 1)) {
          return true;
        }
        break;
      case ADVISOR:
        if (inPalace(color, targetRank, targetFile) && absDf === 1 && absDr === 1) {
          return true;
        }
        break;
      case ELEPHANT: {
        if (absDf === 2 && absDr === 2) {
          const eyeFile = fromFile + (df > 0 ? 1 : -1);
          const eyeRank = fromRank + (dr > 0 ? 1 : -1);
          const eyeSq = eyeRank * 16 + eyeFile;
          if (!board[eyeSq]) {
            return true;
          }
        }
        break;
      }
      case HORSE: {
        if (absDf === 2 && absDr === 1 || absDf === 1 && absDr === 2) {
          let legFile, legRank;
          if (absDf === 2) {
            legFile = fromFile + (df > 0 ? 1 : -1);
            legRank = fromRank;
          } else {
            legFile = fromFile;
            legRank = fromRank + (dr > 0 ? 1 : -1);
          }
          const legSq = legRank * 16 + legFile;
          if (!board[legSq]) {
            return true;
          }
        }
        break;
      }
      case ROOK:
        if (df === 0 || dr === 0) {
          let blocked = false;
          if (df === 0) {
            const step = dr > 0 ? 16 : -16;
            let j = from + step;
            while (j !== to) {
              if (board[j]) {
                blocked = true;
                break;
              }
              j += step;
            }
          } else {
            const step = df > 0 ? 1 : -1;
            let j = from + step;
            while (j !== to) {
              if (board[j]) {
                blocked = true;
                break;
              }
              j += step;
            }
          }
          if (!blocked)
            return true;
        }
        break;
      case CANNON:
        if (df === 0 || dr === 0) {
          let pieceCount = 0;
          if (df === 0) {
            const step = dr > 0 ? 16 : -16;
            let j = from + step;
            while (j !== to) {
              if (board[j])
                pieceCount++;
              j += step;
            }
          } else {
            const step = df > 0 ? 1 : -1;
            let j = from + step;
            while (j !== to) {
              if (board[j])
                pieceCount++;
              j += step;
            }
          }
          if (pieceCount === 1)
            return true;
        }
        break;
      case PAWN: {
        const forward = color === WHITE ? 16 : -16;
        if (targetRank === fromRank && absDf === 1) {
          if (crossedRiver(color, fromRank)) {
            return true;
          }
        } else if (targetFile === fromFile && targetRank === fromRank + (forward >> 4)) {
          return true;
        }
        break;
      }
    }
    return false;
  }
  function isAttacked(board, color, square) {
    for (let i = XQ_SQUARES.a0; i <= XQ_SQUARES.i9; i++) {
      if ((i & 15) >= 9) {
        i += 6;
        continue;
      }
      const piece = board[i];
      if (piece === void 0 || piece.color !== color) {
        continue;
      }
      if (_pieceAttacksSquare(board, piece, color, i, square)) {
        return true;
      }
    }
    return false;
  }
  function isAttackedVerbose(board, color, square) {
    const attackers = [];
    for (let i = XQ_SQUARES.a0; i <= XQ_SQUARES.i9; i++) {
      if ((i & 15) >= 9) {
        i += 6;
        continue;
      }
      const piece = board[i];
      if (piece === void 0 || piece.color !== color) {
        continue;
      }
      if (_pieceAttacksSquare(board, piece, color, i, square)) {
        attackers.push(algebraic(i));
      }
    }
    return attackers;
  }
  function isFlyingGeneral(board, kings) {
    const wk = kings[WHITE];
    const bk = kings[BLACK];
    if (wk === EMPTY || bk === EMPTY)
      return false;
    if (file(wk) !== file(bk))
      return false;
    const minRank = Math.min(rank(wk), rank(bk));
    const maxRank = Math.max(rank(wk), rank(bk));
    const f = file(wk);
    for (let r = minRank + 1; r < maxRank; r++) {
      if (board[r * 16 + f])
        return false;
    }
    return true;
  }
  function isKingAttacked(board, kings, color) {
    const square = kings[color];
    return square === EMPTY ? false : isAttacked(board, swapColor(color), square);
  }
  function addMove(moves, color, from, to, piece, captured, flags = BITS.NORMAL) {
    moves.push({
      color,
      from,
      to,
      piece,
      captured,
      flags
    });
  }
  function generatePseudoMoves(board, turn, _kings, options = {}) {
    const { piece: forPiece, square } = options;
    const forSquare = square ? square.toLowerCase() : void 0;
    const moves = [];
    const us = turn;
    const them = swapColor(us);
    let firstSquare = XQ_SQUARES.a0;
    let lastSquare = XQ_SQUARES.i9;
    if (forSquare) {
      if (!(forSquare in XQ_SQUARES))
        return [];
      firstSquare = lastSquare = XQ_SQUARES[forSquare];
    }
    for (let from = firstSquare; from <= lastSquare; from++) {
      if ((from & 15) >= 9) {
        from += 6;
        continue;
      }
      if (!board[from] || board[from].color !== us)
        continue;
      const { type } = board[from];
      if (forPiece && forPiece !== type)
        continue;
      const fromRank = rank(from);
      switch (type) {
        case KING: {
          for (const offset of PIECE_OFFSETS.k) {
            const to = from + offset;
            if (offBoard(to))
              continue;
            const toR = rank(to);
            const toF = file(to);
            if (!inPalace(us, toR, toF))
              continue;
            if (!board[to]) {
              addMove(moves, us, from, to, KING);
            } else if (board[to].color === them) {
              addMove(moves, us, from, to, KING, board[to].type, BITS.CAPTURE);
            }
          }
          break;
        }
        case ADVISOR: {
          for (const offset of PIECE_OFFSETS.a) {
            const to = from + offset;
            if (offBoard(to))
              continue;
            const toR = rank(to);
            const toF = file(to);
            if (!inPalace(us, toR, toF))
              continue;
            if (!board[to]) {
              addMove(moves, us, from, to, ADVISOR);
            } else if (board[to].color === them) {
              addMove(moves, us, from, to, ADVISOR, board[to].type, BITS.CAPTURE);
            }
          }
          break;
        }
        case ELEPHANT: {
          const onOwnSide = us === WHITE ? fromRank <= 4 : fromRank >= 5;
          if (!onOwnSide)
            break;
          for (const offset of PIECE_OFFSETS[ELEPHANT]) {
            const to = from + offset;
            if (offBoard(to))
              continue;
            const toR = rank(to);
            const toOnOwnSide = us === WHITE ? toR <= 4 : toR >= 5;
            if (!toOnOwnSide)
              continue;
            const eyeSq = from + ELEPHANT_EYES[offset];
            if (board[eyeSq])
              continue;
            if (!board[to]) {
              addMove(moves, us, from, to, ELEPHANT);
            } else if (board[to].color === them) {
              addMove(moves, us, from, to, ELEPHANT, board[to].type, BITS.CAPTURE);
            }
          }
          break;
        }
        case HORSE: {
          for (const offset of PIECE_OFFSETS[HORSE]) {
            const to = from + offset;
            if (offBoard(to))
              continue;
            const legSq = from + HORSE_LEGS[offset];
            if (board[legSq])
              continue;
            if (!board[to]) {
              addMove(moves, us, from, to, HORSE);
            } else if (board[to].color === them) {
              addMove(moves, us, from, to, HORSE, board[to].type, BITS.CAPTURE);
            }
          }
          break;
        }
        case ROOK: {
          for (const offset of PIECE_OFFSETS.r) {
            let to = from + offset;
            while (!offBoard(to)) {
              if (!board[to]) {
                addMove(moves, us, from, to, ROOK);
              } else {
                if (board[to].color === them) {
                  addMove(moves, us, from, to, ROOK, board[to].type, BITS.CAPTURE);
                }
                break;
              }
              to += offset;
            }
          }
          break;
        }
        case CANNON: {
          for (const offset of PIECE_OFFSETS.c) {
            let to = from + offset;
            while (!offBoard(to) && !board[to]) {
              addMove(moves, us, from, to, CANNON);
              to += offset;
            }
            if (!offBoard(to)) {
              to += offset;
              while (!offBoard(to) && !board[to]) {
                to += offset;
              }
              if (!offBoard(to) && board[to]?.color === them) {
                addMove(moves, us, from, to, CANNON, board[to].type, BITS.CAPTURE);
              }
            }
          }
          break;
        }
        case PAWN: {
          const forward = us === WHITE ? 16 : -16;
          const fwd = from + forward;
          if (!offBoard(fwd)) {
            if (!board[fwd]) {
              addMove(moves, us, from, fwd, PAWN);
            } else if (board[fwd].color === them) {
              addMove(moves, us, from, fwd, PAWN, board[fwd].type, BITS.CAPTURE);
            }
          }
          if (crossedRiver(us, fromRank)) {
            for (const side of [-1, 1]) {
              const to = from + side;
              if (offBoard(to) || rank(to) !== fromRank)
                continue;
              if (!board[to]) {
                addMove(moves, us, from, to, PAWN);
              } else if (board[to].color === them) {
                addMove(moves, us, from, to, PAWN, board[to].type, BITS.CAPTURE);
              }
            }
          }
          break;
        }
      }
    }
    return moves;
  }
  var Chess = class {
    _board = new Array(160);
    _turn = WHITE;
    _header = {};
    _kings = { w: EMPTY, b: EMPTY };
    _halfMoves = 0;
    _moveNumber = 0;
    _history = [];
    _comments = {};
    _suffixes = {};
    _nags = {};
    _hash = 0n;
    // tracks number of times a position has been seen for repetition checking
    _positionCount = /* @__PURE__ */ new Map();
    _isCheck = false;
    _isCheckmate = false;
    constructor(fen = DEFAULT_POSITION, { skipValidation = false } = {}) {
      this._comments = {};
      this._suffixes = {};
      this._nags = {};
      this.load(fen, { skipValidation });
    }
    clear({ preserveHeaders = false } = {}) {
      this._board = new Array(160);
      this._kings = { w: EMPTY, b: EMPTY };
      this._turn = WHITE;
      this._halfMoves = 0;
      this._moveNumber = 1;
      this._history = [];
      this._comments = {};
      this._header = preserveHeaders ? this._header : { ...HEADER_TEMPLATE };
      this._hash = computeHash(this._board, this._turn);
      this._positionCount = /* @__PURE__ */ new Map();
      this._header["SetUp"] = null;
      this._header["FEN"] = null;
    }
    load(fen, { skipValidation = false, preserveHeaders = false } = {}) {
      let tokens = fen.split(/\s+/);
      if (tokens.length >= 1 && tokens.length < 6) {
        const adjustments = ["w", "-", "-", "0", "1"];
        fen = tokens.concat(adjustments.slice(-(6 - tokens.length))).join(" ");
      }
      tokens = fen.split(/\s+/);
      if (!skipValidation) {
        const { ok, error } = validateFen(fen);
        if (!ok) {
          throw new Error(error);
        }
      }
      const parsed = parseFen(fen);
      this.clear({ preserveHeaders });
      this._board = parsed.board;
      for (let i = XQ_SQUARES.a0; i <= XQ_SQUARES.i9; i++) {
        if ((i & 15) >= 9) {
          i += 6;
          continue;
        }
        if (this._board[i] && this._board[i].type === KING) {
          this._kings[this._board[i].color] = i;
        }
      }
      this._turn = parsed.turn;
      this._halfMoves = parsed.halfMoves;
      this._moveNumber = parsed.moveNumber;
      this._hash = computeHash(this._board, this._turn);
      this._updateSetup(fen);
      this._incPositionCount();
    }
    fen() {
      return serializeFen(this._board, this._turn, this._halfMoves, this._moveNumber);
    }
    /*
     * Called when the initial board setup is changed with put() or remove().
     */
    _updateSetup(fen) {
      if (this._history.length > 0)
        return;
      if (fen !== DEFAULT_POSITION) {
        this._header["SetUp"] = "1";
        this._header["FEN"] = fen;
      } else {
        this._header["SetUp"] = null;
        this._header["FEN"] = null;
      }
    }
    reset() {
      this.load(DEFAULT_POSITION);
      this._comments = {};
      this._suffixes = {};
      this._nags = {};
    }
    get(square) {
      return this._board[XQ_SQUARES[square]];
    }
    findPiece(piece) {
      const squares = [];
      for (let i = XQ_SQUARES.a0; i <= XQ_SQUARES.i9; i++) {
        if ((i & 15) >= 9) {
          i += 6;
          continue;
        }
        if (!this._board[i] || this._board[i]?.color !== piece.color) {
          continue;
        }
        if (this._board[i].color === piece.color && this._board[i].type === piece.type) {
          squares.push(algebraic(i));
        }
      }
      return squares;
    }
    put({ type, color }, square) {
      if (this._put({ type, color }, square)) {
        this._updateSetup(this.fen());
        return true;
      }
      return false;
    }
    _set(sq2, piece) {
      this._hash ^= pieceKey(this._board, sq2);
      this._board[sq2] = piece;
      this._hash ^= pieceKey(this._board, sq2);
    }
    _put({ type, color }, square) {
      if (SYMBOLS.indexOf(type.toLowerCase()) === -1) {
        return false;
      }
      if (!(square in XQ_SQUARES)) {
        return false;
      }
      const sq2 = XQ_SQUARES[square];
      if (type == KING && !(this._kings[color] == EMPTY || this._kings[color] == sq2)) {
        return false;
      }
      const currentPieceOnSquare = this._board[sq2];
      if (currentPieceOnSquare && currentPieceOnSquare.type === KING) {
        this._kings[currentPieceOnSquare.color] = EMPTY;
      }
      this._set(sq2, { type, color });
      if (type === KING) {
        this._kings[color] = sq2;
      }
      return true;
    }
    _clear(sq2) {
      this._hash ^= pieceKey(this._board, sq2);
      delete this._board[sq2];
    }
    remove(square) {
      const piece = this.get(square);
      this._clear(XQ_SQUARES[square]);
      if (piece && piece.type === KING) {
        this._kings[piece.color] = EMPTY;
      }
      this._updateSetup(this.fen());
      return piece;
    }
    attackers(square, attackedBy) {
      const color = attackedBy ?? this._turn;
      return isAttackedVerbose(this._board, color, XQ_SQUARES[square]);
    }
    hash() {
      return this._hash.toString(16);
    }
    isAttacked(square, attackedBy) {
      return isAttacked(this._board, attackedBy, XQ_SQUARES[square]);
    }
    isCheck() {
      return isKingAttacked(this._board, this._kings, this._turn);
    }
    inCheck() {
      return this.isCheck();
    }
    // In Xiangqi, having no legal moves is a loss (not stalemate)
    isCheckmate() {
      return this._moves().length === 0;
    }
    // In Xiangqi, stalemate does not exist - no legal moves is a loss
    isStalemate() {
      return false;
    }
    isInsufficientMaterial() {
      const pieces = {};
      let numPieces = 0;
      for (let i = XQ_SQUARES.a0; i <= XQ_SQUARES.i9; i++) {
        if ((i & 15) >= 9) {
          i += 6;
          continue;
        }
        if (this._board[i]) {
          pieces[this._board[i].type] = (pieces[this._board[i].type] || 0) + 1;
          numPieces++;
        }
      }
      if (numPieces === 2)
        return true;
      const attackPieces = [
        pieces[ROOK] || 0,
        pieces[CANNON] || 0,
        pieces[HORSE] || 0,
        pieces[PAWN] || 0
      ];
      const hasAttackPieces = attackPieces.some((c) => c > 0);
      if (!hasAttackPieces) {
        if ((pieces[ADVISOR] || 0) + (pieces[ELEPHANT] || 0) <= 2) {
          return true;
        }
      }
      return false;
    }
    isThreefoldRepetition() {
      return this._getPositionCount(this._hash) >= 3;
    }
    isDrawByFiftyMoves() {
      return this._halfMoves >= 100;
    }
    isDraw() {
      return this.isDrawByFiftyMoves() || this.isInsufficientMaterial() || this.isThreefoldRepetition();
    }
    isGameOver() {
      return this.isCheckmate() || this.isDraw();
    }
    _createMove(internal) {
      const wxf = moveToWxf(this._board, internal);
      const before = this.fen();
      this._makeMove(internal);
      const after = this.fen();
      const isCheck = this._isCheck;
      const isCheckmate = isCheck && this.isCheckmate();
      this._undoMove();
      const zh = moveToZh(wxf, internal.color);
      const iccs = moveToIccs(internal);
      return new Move(internal, zh, before, after, wxf, iccs, isCheck, isCheckmate);
    }
    moves({ verbose = false, chinese = false, square = void 0, piece = void 0 } = {}) {
      const moves = this._moves({ square, piece });
      if (verbose) {
        return moves.map((move) => this._createMove(move));
      }
      const toZh = chinese ? (move) => moveToZh(moveToWxf(this._board, move), move.color) : (move) => moveToLan(move);
      return moves.map(toZh);
    }
    _moves({ legal = true, piece: forPiece = void 0, square = void 0 } = {}) {
      const pseudo = generatePseudoMoves(this._board, this._turn, this._kings, {
        piece: forPiece,
        square
      });
      if (!legal || this._kings[this._turn] === EMPTY)
        return pseudo;
      const us = this._turn;
      const legalMoves = [];
      for (const m of pseudo) {
        this._makeMove(m);
        if (!isKingAttacked(this._board, this._kings, us) && !isFlyingGeneral(this._board, this._kings)) {
          legalMoves.push(m);
        }
        this._undoMove();
      }
      return legalMoves;
    }
    move(move, { strict = false } = {}) {
      let moveObj = null;
      if (typeof move === "string") {
        moveObj = moveFromSan(move, this._turn, this._moves(), strict);
      } else if (move === null) {
        moveObj = moveFromSan(SAN_NULLMOVE, this._turn, this._moves(), strict);
      } else if (typeof move === "object") {
        const moves = this._moves();
        for (let i = 0; i < moves.length; i++) {
          if (move.from === algebraic(moves[i].from) && move.to === algebraic(moves[i].to)) {
            moveObj = moves[i];
            break;
          }
        }
      }
      if (!moveObj) {
        if (typeof move === "string") {
          throw new Error(`Invalid move: ${move}`);
        } else {
          throw new Error(`Invalid move: ${JSON.stringify(move)}`);
        }
      }
      if (this.isCheck() && moveObj.flags & BITS.NULL_MOVE) {
        throw new Error("Null move not allowed when in check");
      }
      const prettyMove = this._createMove(moveObj);
      this._makeMove(moveObj);
      this._incPositionCount();
      return prettyMove;
    }
    _push(move) {
      this._history.push({
        move,
        kings: { b: this._kings.b, w: this._kings.w },
        turn: this._turn,
        halfMoves: this._halfMoves,
        moveNumber: this._moveNumber
      });
    }
    _movePiece(from, to) {
      this._hash ^= pieceKey(this._board, from);
      this._board[to] = this._board[from];
      delete this._board[from];
      this._hash ^= pieceKey(this._board, to);
    }
    _makeMove(move) {
      const us = this._turn;
      const them = swapColor(us);
      this._push(move);
      if (move.flags & BITS.NULL_MOVE) {
        if (us === BLACK) {
          this._moveNumber++;
        }
        this._halfMoves++;
        this._turn = them;
        return;
      }
      if (move.captured) {
        this._hash ^= pieceKey(this._board, move.to);
      }
      this._movePiece(move.from, move.to);
      if (this._board[move.to].type === KING) {
        this._kings[us] = move.to;
      }
      if (move.piece === PAWN || move.flags & BITS.CAPTURE) {
        this._halfMoves = 0;
      } else {
        this._halfMoves++;
      }
      if (us === BLACK) {
        this._moveNumber++;
      }
      this._turn = them;
      this._hash ^= SIDE_KEY;
      this._isCheck = isKingAttacked(this._board, this._kings, them);
      this._isCheckmate = false;
    }
    undo() {
      const hash = this._hash;
      const move = this._undoMove();
      if (move) {
        const prettyMove = this._createMove(move);
        this._decPositionCount(hash);
        return prettyMove;
      }
      return null;
    }
    _undoMove() {
      const old = this._history.pop();
      if (old === void 0) {
        return null;
      }
      const move = old.move;
      this._kings = old.kings;
      this._turn = old.turn;
      this._halfMoves = old.halfMoves;
      this._moveNumber = old.moveNumber;
      this._hash ^= SIDE_KEY;
      const us = this._turn;
      const them = swapColor(us);
      if (move.flags & BITS.NULL_MOVE) {
        return move;
      }
      this._movePiece(move.to, move.from);
      if (move.piece) {
        this._clear(move.from);
        this._set(move.from, { type: move.piece, color: us });
      }
      if (move.captured) {
        this._set(move.to, { type: move.captured, color: them });
      }
      this._isCheck = isKingAttacked(this._board, this._kings, this._turn);
      this._isCheckmate = false;
      return move;
    }
    pgn({ newline = "\n", maxWidth = 0 } = {}) {
      const result = [];
      let headerExists = false;
      for (const i in this._header) {
        const headerTag = this._header[i];
        if (headerTag)
          result.push(`[${i} "${this._header[i]}"]` + newline);
        headerExists = true;
      }
      if (headerExists && this._history.length) {
        result.push(newline);
      }
      const appendComment = (moveString2) => {
        const comment = this._comments[this.fen()];
        if (typeof comment !== "undefined") {
          const delimiter = moveString2.length > 0 ? " " : "";
          moveString2 = `${moveString2}${delimiter}{${comment}}`;
        }
        return moveString2;
      };
      const reversedHistory = [];
      while (this._history.length > 0) {
        reversedHistory.push(this._undoMove());
      }
      const moves = [];
      let moveString = "";
      if (reversedHistory.length === 0) {
        moves.push(appendComment(""));
      }
      while (reversedHistory.length > 0) {
        moveString = appendComment(moveString);
        const move = reversedHistory.pop();
        if (!move)
          break;
        if (!this._history.length && move.color === "b") {
          const prefix = `${this._moveNumber}. ...`;
          moveString = moveString ? `${moveString} ${prefix}` : prefix;
        } else if (move.color === "w") {
          if (moveString.length) {
            moves.push(moveString);
          }
          moveString = this._moveNumber + ".";
        }
        moveString = moveString + " " + moveToLan(move);
        this._makeMove(move);
      }
      if (moveString.length) {
        moves.push(appendComment(moveString));
      }
      moves.push(this._header.Result || "*");
      if (maxWidth === 0) {
        return result.join("") + moves.join(" ");
      }
      const strip = function() {
        if (result.length > 0 && result[result.length - 1] === " ") {
          result.pop();
          return true;
        }
        return false;
      };
      const wrapComment = function(width, move) {
        for (const token of move.split(" ")) {
          if (!token)
            continue;
          if (width + token.length > maxWidth) {
            while (strip())
              width--;
            result.push(newline);
            width = 0;
          }
          result.push(token);
          width += token.length;
          result.push(" ");
          width++;
        }
        if (strip())
          width--;
        return width;
      };
      let currentWidth = 0;
      for (let i = 0; i < moves.length; i++) {
        if (currentWidth + moves[i].length > maxWidth) {
          if (moves[i].includes("{")) {
            currentWidth = wrapComment(currentWidth, moves[i]);
            continue;
          }
        }
        if (currentWidth + moves[i].length > maxWidth && i !== 0) {
          if (result[result.length - 1] === " ") {
            result.pop();
          }
          result.push(newline);
          currentWidth = 0;
        } else if (i !== 0) {
          result.push(" ");
          currentWidth++;
        }
        result.push(moves[i]);
        currentWidth += moves[i].length;
      }
      return result.join("");
    }
    header(...args) {
      for (let i = 0; i < args.length; i += 2) {
        if (typeof args[i] === "string" && typeof args[i + 1] === "string") {
          this._header[args[i]] = args[i + 1];
        }
      }
      return this._header;
    }
    setHeader(key, value) {
      this._header[key] = value ?? SEVEN_TAG_ROSTER[key] ?? null;
      return this.getHeaders();
    }
    removeHeader(key) {
      if (key in this._header) {
        this._header[key] = SEVEN_TAG_ROSTER[key] || null;
        return true;
      }
      return false;
    }
    getHeaders() {
      const nonNullHeaders = {};
      for (const [key, value] of Object.entries(this._header)) {
        if (value !== null) {
          nonNullHeaders[key] = value;
        }
      }
      return nonNullHeaders;
    }
    loadPgn(pgn2, { strict = false, newlineChar = "\r?\n" } = {}) {
      if (newlineChar !== "\r?\n") {
        pgn2 = pgn2.replace(new RegExp(newlineChar, "g"), "\n");
      }
      const parsedPgn = peg$parse(pgn2);
      this.reset();
      const headers = parsedPgn.headers;
      let fen = "";
      for (const key in headers) {
        if (key.toLowerCase() === "fen") {
          fen = headers[key];
        }
        this.header(key, headers[key]);
      }
      if (!strict) {
        if (fen) {
          this.load(fen, { preserveHeaders: true });
        }
      } else {
        if (headers["SetUp"] === "1") {
          if (!("FEN" in headers)) {
            throw new Error("Invalid PGN: FEN tag must be supplied with SetUp tag");
          }
          this.load(headers["FEN"], { preserveHeaders: true });
        }
      }
      let node2 = parsedPgn.root;
      while (node2) {
        if (node2.move) {
          const suffixAnnotation = node2.suffixAnnotation;
          const move = moveFromSan(node2.move, this._turn, this._moves(), strict);
          if (!move) {
            throw new Error(`Invalid move in PGN: ${node2.move}`);
          } else {
            this._makeMove(move);
            this._incPositionCount();
            if (suffixAnnotation) {
              this._suffixes[this.fen()] = suffixAnnotation;
            }
            if (node2.nags && node2.nags.length > 0) {
              this._nags[this.fen()] = node2.nags;
            }
          }
        }
        if (node2.comment !== void 0) {
          this._comments[this.fen()] = node2.comment;
        }
        node2 = node2.variations[0];
      }
      const result = parsedPgn.result;
      if (result && Object.keys(this._header).length && this._header["Result"] !== result) {
        this.setHeader("Result", result);
      }
    }
    ascii() {
      let s = "   +---+---+---+---+---+---+---+---+---+\n";
      for (let r = 9; r >= 0; r--) {
        s += " " + r + " |";
        for (let f = 0; f < 9; f++) {
          const sq2 = r * 16 + f;
          if (this._board[sq2]) {
            const piece = this._board[sq2].type;
            const color = this._board[sq2].color;
            const symbol = color === WHITE ? piece.toUpperCase() : piece.toLowerCase();
            s += " " + symbol + " ";
          } else {
            s += " . ";
          }
          if (f < 8)
            s += "|";
        }
        s += "|\n";
        if (r === 5) {
          s += "   +~~~+~~~+~~~+~~~+~~~+~~~+~~~+~~~+~~~+\n";
        } else if (r > 0) {
          s += "   +---+---+---+---+---+---+---+---+---+\n";
        }
      }
      s += "   +---+---+---+---+---+---+---+---+---+\n";
      s += "     a   b   c   d   e   f   g   h   i";
      return s;
    }
    perft(depth) {
      const moves = generatePseudoMoves(this._board, this._turn, this._kings);
      let nodes = 0;
      const color = this._turn;
      for (let i = 0; i < moves.length; i++) {
        this._makeMove(moves[i]);
        if (!isKingAttacked(this._board, this._kings, color) && !isFlyingGeneral(this._board, this._kings)) {
          if (depth - 1 > 0) {
            nodes += this.perft(depth - 1);
          } else {
            nodes++;
          }
        }
        this._undoMove();
      }
      return nodes;
    }
    setTurn(color) {
      if (this._turn == color) {
        return false;
      }
      this.move(SAN_NULLMOVE);
      return true;
    }
    turn() {
      return this._turn;
    }
    board() {
      const output = [];
      for (let r = 9; r >= 0; r--) {
        const row = [];
        for (let f = 0; f < 9; f++) {
          const i = r * 16 + f;
          if (this._board[i] == null) {
            row.push(null);
          } else {
            row.push({
              square: algebraic(i),
              type: this._board[i].type,
              color: this._board[i].color
            });
          }
        }
        output.push(row);
      }
      return output;
    }
    squareColor(square) {
      if (square in XQ_SQUARES) {
        const sq2 = XQ_SQUARES[square];
        return (rank(sq2) + file(sq2)) % 2 === 0 ? "light" : "dark";
      }
      return null;
    }
    history({ verbose = false } = {}) {
      const reversedHistory = [];
      const moveHistory = [];
      while (this._history.length > 0) {
        reversedHistory.push(this._undoMove());
      }
      while (true) {
        const move = reversedHistory.pop();
        if (!move)
          break;
        if (verbose) {
          moveHistory.push(this._createMove(move));
        } else {
          moveHistory.push(moveToLan(move));
        }
        this._makeMove(move);
      }
      return moveHistory;
    }
    _getPositionCount(hash) {
      return this._positionCount.get(hash) ?? 0;
    }
    _incPositionCount() {
      this._positionCount.set(this._hash, (this._positionCount.get(this._hash) ?? 0) + 1);
    }
    _decPositionCount(hash) {
      const currentCount = this._positionCount.get(hash) ?? 0;
      if (currentCount === 1) {
        this._positionCount.delete(hash);
      } else {
        this._positionCount.set(hash, currentCount - 1);
      }
    }
    _pruneComments() {
      const reversedHistory = [];
      const currentComments = {};
      const copyComment = (fen) => {
        if (fen in this._comments) {
          currentComments[fen] = this._comments[fen];
        }
      };
      while (this._history.length > 0) {
        reversedHistory.push(this._undoMove());
      }
      copyComment(this.fen());
      while (true) {
        const move = reversedHistory.pop();
        if (!move)
          break;
        this._makeMove(move);
        copyComment(this.fen());
      }
      this._comments = currentComments;
    }
    getComment() {
      return this._comments[this.fen()];
    }
    setComment(comment) {
      this._comments[this.fen()] = comment.replace("{", "[").replace("}", "]");
    }
    deleteComment() {
      return this.removeComment();
    }
    removeComment() {
      const comment = this._comments[this.fen()];
      delete this._comments[this.fen()];
      return comment;
    }
    getComments() {
      this._pruneComments();
      const allFenKeys = /* @__PURE__ */ new Set();
      Object.keys(this._comments).forEach((fen) => allFenKeys.add(fen));
      Object.keys(this._suffixes).forEach((fen) => allFenKeys.add(fen));
      Object.keys(this._nags).forEach((fen) => allFenKeys.add(fen));
      const result = [];
      for (const fen of allFenKeys) {
        const commentContent = this._comments[fen];
        const suffixAnnotation = this._suffixes[fen];
        const nags = this._nags[fen];
        const entry2 = {
          fen,
          nags: nags ?? []
        };
        if (commentContent !== void 0) {
          entry2.comment = commentContent;
        }
        if (suffixAnnotation !== void 0) {
          entry2.suffixAnnotation = suffixAnnotation;
        }
        result.push(entry2);
      }
      return result;
    }
    getSuffixAnnotation(fen) {
      const key = fen ?? this.fen();
      return this._suffixes[key];
    }
    setSuffixAnnotation(suffix, fen) {
      if (!SUFFIX_LIST.includes(suffix)) {
        throw new Error(`Invalid suffix: ${suffix}`);
      }
      this._suffixes[fen || this.fen()] = suffix;
    }
    removeSuffixAnnotation(fen) {
      const key = fen || this.fen();
      const old = this._suffixes[key];
      delete this._suffixes[key];
      return old;
    }
    getNags(fen) {
      const key = fen ?? this.fen();
      return this._nags[key] ?? [];
    }
    addNag(nag, fen) {
      const key = fen || this.fen();
      if (!this._nags[key]) {
        this._nags[key] = [];
      }
      if (!this._nags[key].includes(nag)) {
        this._nags[key].push(nag);
      }
    }
    setNags(nags, fen) {
      const key = fen || this.fen();
      this._nags[key] = [...nags];
    }
    removeNags(fen) {
      const key = fen || this.fen();
      const old = this._nags[key] ?? [];
      delete this._nags[key];
      return old;
    }
    removeNag(nag, fen) {
      const key = fen || this.fen();
      const nags = this._nags[key];
      if (!nags)
        return false;
      const index = nags.indexOf(nag);
      if (index === -1)
        return false;
      nags.splice(index, 1);
      if (nags.length === 0) {
        delete this._nags[key];
      }
      return true;
    }
    deleteComments() {
      return this.removeComments();
    }
    removeComments() {
      this._pruneComments();
      return Object.keys(this._comments).map((fen) => {
        const comment = this._comments[fen];
        delete this._comments[fen];
        return { fen, comment };
      });
    }
    moveNumber() {
      return this._moveNumber;
    }
  };

  // kernel/upstream/chess.ts
  var DEFAULT_FEN = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w";
  function getSaveNotation(move) {
    return move.iccs ?? move.zh ?? "";
  }
  function isMoveCheckmate(move) {
    return move.isCheckmate ?? false;
  }
  function getPgnVariantTag(_fen) {
    return null;
  }
  var MOVE_REGEX = /^[A-Ia-i][0-9]-?[A-Ia-i][0-9]/;
  var FEN_REGEX = /^[rnbakcpRNBAKCP1-9]+(\/[rnbakcpRNBAKCP1-9]+){8,9}(\s+[wr])?/;
  function getMoveTokenType(value) {
    if (/^[兵卒车马炮相士帅将车][一二三四五六七八九123456789进退平前后左右]/.test(
      value
    )) {
      return "wxf-move";
    }
    return "iccs-move";
  }
  var PRIMARY_MOVE_TOKEN_TYPES = [
    "iccs-move",
    "wxf-move"
  ];
  function parseMoveInGame(chess, token, _tokenType) {
    return chess.move(token) ?? null;
  }
  var SQUARE_PATTERN = "[a-i][0-9]";
  var SHAPE_PART_REGEX = /^([a-i][0-9])([a-i][0-9])?:([gryb])$/;
  var EVAL_REGEX = /^%e:([m+-]?\d+(?:\.\d+)?|[m+-]?[+-]\d+(?:\.\d+)?),?([a-i0-9]+)?,?([a-i0-9]+)?,?(!\?|\?!|\?\?|[?!]|!!)?$/;
  function getTurnFromFen(fen) {
    return fen.split(" ")[1] === "b" ? "black" : "white";
  }

  // kernel/upstream/utils/winningChances.ts
  var GLYPH_DEFS = {
    "?!": { symbol: "?!", name: "Inaccuracy", color: "#56b4e9" },
    "?": { symbol: "?", name: "Mistake", color: "#e69f00" },
    "??": { symbol: "??", name: "Blunder", color: "#df5353" },
    "!": { symbol: "!", name: "Good move", color: "#22ac38" },
    "!!": { symbol: "!!", name: "Brilliant", color: "#168226" },
    "!?": { symbol: "!?", name: "Interesting", color: "#ea45d8" }
  };
  var NAG_TO_SYMBOL = {
    $1: "!",
    $2: "?",
    $3: "!!",
    $4: "??",
    $5: "!?",
    $6: "?!"
  };
  function nagToGlyph(nag) {
    const symbol = nag.startsWith("$") ? NAG_TO_SYMBOL[nag] : nag;
    const def = symbol ? GLYPH_DEFS[symbol] : void 0;
    return def ?? null;
  }

  // kernel/upstream/utils/annotation-glyphs.ts
  var entry = (nag, symbol, category, nameKey, color) => ({
    key: String(nag),
    nag,
    symbol,
    category,
    nameKey,
    color
  });
  var STUDY_GLYPHS = [
    // -- move quality (exclusive, stored in node.glyph) --
    entry(1, "!", "move", "annotation.good", "#22ac38"),
    entry(2, "?", "move", "annotation.mistake", "#e69f00"),
    entry(3, "!!", "move", "annotation.brilliant", "#168226"),
    entry(4, "??", "move", "annotation.blunder", "#df5353"),
    entry(5, "!?", "move", "annotation.interesting", "#ea45d8"),
    entry(6, "?!", "move", "annotation.dubious", "#56b4e9"),
    // -- position (mutually exclusive, trimmed to the four essentials) --
    // Badge colours mirror the advantage/disadvantage/equal icon trio
    // (src/utils/icon.ts BADGE_DEFS); $13 keeps lila's badge colour.
    entry(10, "=", "position", "annotation.equal", "#82c2ef"),
    entry(13, "\u221E", "position", "annotation.unclear", "#f5918f"),
    entry(16, "\xB1", "position", "annotation.wBetter", "#22ac38"),
    entry(17, "\u2213", "position", "annotation.bBetter", "#df5353"),
    // -- legacy custom annotations (kept verbatim, private #a: format) --
    {
      key: "bm",
      symbol: "bm",
      category: "custom",
      nameKey: "annotation.bm",
      color: "#e69f00",
      icon: "bookmark"
    },
    {
      key: "st",
      symbol: "st",
      category: "custom",
      nameKey: "annotation.br",
      color: "#22ac38",
      icon: "star"
    }
  ];
  var GLYPH_BY_KEY = Object.fromEntries(
    STUDY_GLYPHS.map((g) => [g.key, g])
  );
  var GLYPH_BY_SYMBOL = Object.fromEntries(
    STUDY_GLYPHS.filter((g) => g.category !== "custom").map((g) => [
      g.symbol,
      g
    ])
  );
  var LEGACY_NAG_KEYS = {
    $24: "st"
  };
  var QUALITY_KEYS = /* @__PURE__ */ new Set(["1", "2", "3", "4", "5", "6"]);
  function nagToAnnotationKey(nag) {
    const legacy = LEGACY_NAG_KEYS[nag];
    if (legacy) return legacy;
    const key = nag.replace(/^\$/, "");
    const def = GLYPH_BY_KEY[key];
    return def && def.nag && !QUALITY_KEYS.has(key) ? key : null;
  }
  var LEGACY_CUSTOM_KEYS = {
    "+": "16",
    "-": "17",
    "=": "10"
  };
  function pushAnnotation(node2, rawKey) {
    const key = LEGACY_CUSTOM_KEYS[rawKey] ?? rawKey;
    if (!GLYPH_BY_KEY[key]) return;
    node2.annotations ??= [];
    if (!node2.annotations.includes(key)) node2.annotations.push(key);
  }

  // kernel/upstream/utils/comment-meta.ts
  var CSL_REGEX = new RegExp(
    `\\[\\s*%csl\\s+((?:\\w${SQUARE_PATTERN}[\\s,]*)+)\\]`,
    "g"
  );
  var CAL_REGEX = new RegExp(
    `\\[\\s*%cal\\s+((?:\\w${SQUARE_PATTERN}${SQUARE_PATTERN}[\\s,]*)+)\\]`,
    "g"
  );
  var EVAL_TAG_REGEX = /\[\s*%eval\s+([^\]]*?)\]/g;
  var CLK_REGEX = /\[\s*%clk\s+(\d+(?::\d{1,2}){1,2})\]/g;
  var ANNO_REGEX = /\[\s*%anno\s+(?:"([^"]*)"|([^\],]+))\s*(?:,\s*([^\]\s]+))?\s*\]/g;
  var BRUSH_BY_COLOR = {
    G: "g",
    R: "r",
    Y: "y",
    B: "b"
  };
  function toBrush(color) {
    return BRUSH_BY_COLOR[color] ?? "b";
  }
  function parseLichessEval(value) {
    const str = value.trim();
    const mateMatch = str.match(/^[#M]([+-]?\d+)$/);
    if (mateMatch) {
      return { score: Number.parseInt(mateMatch[1], 10), scoreType: "mate" };
    }
    const cpMatch = str.match(/^([+-]?\d+(?:\.\d+)?)$/);
    if (cpMatch) {
      return {
        score: Math.round(Number.parseFloat(cpMatch[1]) * 100),
        scoreType: "cp"
      };
    }
    return void 0;
  }
  function extractCommentMeta(raw) {
    const shapes = [];
    let stripped = false;
    const consumeShapes = (group, squareCount) => {
      const partRegex = new RegExp(
        `\\w(?:${SQUARE_PATTERN}){${squareCount}}`,
        "g"
      );
      for (const part of group.match(partRegex) ?? []) {
        shapes.push({
          orig: part.slice(1, 3),
          ...squareCount > 1 ? { dest: part.slice(3, 5) } : {},
          brush: toBrush(part[0])
        });
      }
      return "";
    };
    let text = raw.replace(CSL_REGEX, (_match, group) => {
      stripped = true;
      return consumeShapes(group, 1);
    });
    text = text.replace(CAL_REGEX, (_match, group) => {
      stripped = true;
      return consumeShapes(group, 2);
    });
    let evalMeta;
    text = text.replace(EVAL_TAG_REGEX, (match, value) => {
      const parsed = parseLichessEval(value);
      if (!parsed) return match;
      stripped = true;
      evalMeta = parsed;
      return "";
    });
    let clock;
    text = text.replace(CLK_REGEX, (_match, value) => {
      stripped = true;
      clock = value;
      return "";
    });
    let annotator;
    text = text.replace(
      ANNO_REGEX,
      (match, quoted, bare, user) => {
        const name = (quoted ?? bare ?? "").trim();
        if (!name && !user) return match;
        stripped = true;
        annotator = { name, user: user ?? "" };
        return "";
      }
    );
    if (stripped) {
      text = text.replace(/\s+/g, " ").trim();
    }
    return { shapes, eval: evalMeta, clock, annotator, text };
  }

  // kernel/upstream/utils/icon.ts
  var ANNOTATION_KEYS = ["+", "-", "=", "bm", "st"];
  var ANNOTATION_PREFIX = "#a:";
  var SHAPES_PREFIX = "&s:";
  function isAnnotationKey(key) {
    return ANNOTATION_KEYS.includes(key);
  }

  // kernel/upstream/modules/Source/Tokenizer.ts
  function tokenize(pgn2) {
    const tokens = [];
    let line = 1;
    let column = 1;
    let pos = 0;
    const advance = (n) => {
      while (n-- > 0) {
        const c = pgn2[pos++];
        if (c === "\n") {
          line++;
          column = 1;
        } else {
          column++;
        }
      }
    };
    const matchAndConsume = (regex) => {
      const match = regex.exec(pgn2.slice(pos));
      if (!match) return null;
      const value = match[0];
      advance(value.length);
      return value;
    };
    while (pos < pgn2.length) {
      const startLine = line;
      const startCol = column;
      const rest = pgn2.slice(pos);
      const char = rest[0];
      if (/^[\s\uFEFF]/.test(rest)) {
        advance(1);
        continue;
      }
      const step = matchAndConsume(/^\d+\s*(?:\.\s*)+/);
      if (step) {
        continue;
      }
      const nag = matchAndConsume(/^\$\d+/);
      if (nag) {
        tokens.push({
          type: "nag",
          value: nag,
          line: startLine,
          column: startCol
        });
        continue;
      }
      if (matchAndConsume(/^%.*/)) {
        continue;
      }
      const move = matchAndConsume(MOVE_REGEX);
      if (move) {
        tokens.push({
          type: getMoveTokenType(move),
          value: move,
          line: startLine,
          column: startCol
        });
        for (; ; ) {
          const suffixLine = line;
          const suffixCol = column;
          const suffix = matchAndConsume(/^(!!|\?\?|!\?|\?!|[!?])/);
          if (!suffix) break;
          tokens.push({
            type: "nag",
            value: suffix,
            line: suffixLine,
            column: suffixCol
          });
        }
        continue;
      }
      if (char === "{") {
        let depth = 1;
        let end = pos + 1;
        while (end < pgn2.length && depth > 0) {
          if (pgn2[end] === "{") depth++;
          else if (pgn2[end] === "}") depth--;
          end++;
        }
        const comment = pgn2.slice(pos, end);
        advance(end - pos);
        tokens.push({
          type: "comment",
          value: comment,
          line: startLine,
          column: startCol
        });
        continue;
      }
      const tag = matchAndConsume(/^\[[^\]]*\]/);
      if (tag) {
        tokens.push({
          type: "tag",
          value: tag,
          line: startLine,
          column: startCol
        });
        continue;
      }
      const result = matchAndConsume(/^(1-0|0-1|1\/2-1\/2|\*)/);
      if (result) {
        tokens.push({
          type: "result",
          value: result,
          line: startLine,
          column: startCol
        });
        continue;
      }
      if (char === "(") {
        advance(1);
        tokens.push({
          type: "left-paren",
          value: "(",
          line: startLine,
          column: startCol
        });
        continue;
      }
      if (char === ")") {
        advance(1);
        tokens.push({
          type: "right-paren",
          value: ")",
          line: startLine,
          column: startCol
        });
        continue;
      }
      const fen = matchAndConsume(FEN_REGEX);
      if (fen) {
        tokens.push({
          type: "tag",
          value: `[FEN "${fen}"]`,
          line: startLine,
          column: startCol
        });
        continue;
      }
      tokens.push({
        type: "unknown",
        value: char,
        line: startLine,
        column: startCol
      });
      advance(1);
    }
    tokens.push({ type: "eof", value: "", line, column });
    return tokens;
  }

  // kernel/upstream/modules/Source/parser.ts
  var PGNParser = class {
    constructor(input, strict = false) {
      this.strict = strict;
      this.nodeMap = /* @__PURE__ */ new Map();
      this.tokens = typeof input === "string" ? tokenize(input) : input;
      this.currentIndex = 0;
      this.nodeId = 1;
      this.chess = new Chess(DEFAULT_FEN);
      this.rootNode = {
        id: `node-root`,
        fen: DEFAULT_FEN,
        move: null,
        step: 0,
        color: null,
        parentID: null,
        children: [],
        comments: []
      };
      this.nodeMap.set(this.rootNode.id, this.rootNode);
      this.currentStep++;
      this.currentNode = this.rootNode;
      while (!this.match("eof") && !this.parseAborted) {
        if (this.match("tag")) {
          this.parseTag();
        } else if (this.isMoveToken()) {
          this.processMove(
            this.consume().value,
            this.tokens[this.currentIndex - 1].type
          );
        } else if (this.match("left-paren")) {
          this.parseVariation();
        } else if (this.match("comment")) {
          this.parseComment();
        } else if (this.match("nag")) {
          this.parseNag();
        } else if (this.match("result")) {
          this.parseResult();
        } else if (this.match("unknown")) {
          this.parseUnknown();
        } else {
          this.consume();
        }
      }
      if (this.strict && this.nodeMap.size <= 1) {
        throw new Error("PGN contains no legal moves");
      }
      const headerResult = this.tags.get("Result");
      if (headerResult === "1-0" || headerResult === "0-1" || headerResult === "1/2-1/2") {
        let tip = this.rootNode;
        while (tip.children.length > 0) tip = tip.children[0];
        tip.result ??= headerResult;
      }
    }
    strict;
    haveFEN = false;
    tokens;
    nodeMap;
    currentIndex;
    rootNode;
    currentNode;
    nodeId;
    currentStep = 0;
    tags = /* @__PURE__ */ new Map();
    chess;
    skippedTokens = [];
    parseAborted = false;
    parseUnknown() {
      const first = this.peek();
      let text = "";
      let endLine = first.line;
      let endColumn = first.column;
      while (this.match("unknown")) {
        const tok = this.peek();
        if (text && (tok.line !== endLine || tok.column !== endColumn)) break;
        text += tok.value;
        endLine = tok.line;
        endColumn = tok.column + tok.value.length;
        this.consume();
      }
      if (this.strict) throw new Error(`Unrecognized content: ${text}`);
      this.skippedTokens.push({
        text,
        line: first.line,
        column: first.column,
        kind: "unknown"
      });
      this.parseAborted = true;
    }
    isMoveToken() {
      const type = this.peek().type;
      return PRIMARY_MOVE_TOKEN_TYPES.includes(type);
    }
    parseTag() {
      const token = this.consume();
      const tagText = token.value;
      const match = tagText.match(/^\[(\w+)\s+"([^"]*)"\]$/);
      if (!match) return;
      const [, tagName, tagValue] = match;
      this.tags.set(tagName, tagValue);
      if (tagName.toUpperCase() === "FEN") {
        try {
          this.chess.load(tagValue);
          this.rootNode.fen = tagValue;
          this.haveFEN = true;
        } catch {
          if (this.strict) throw new Error(`Invalid FEN tag: ${tagValue}`);
          this.skippedTokens.push({
            text: `[FEN "${tagValue}"]`,
            line: token.line,
            column: token.column,
            kind: "fen"
          });
          this.parseAborted = true;
        }
      }
    }
    createNode(move, fen) {
      const color = move.color === "w" ? "white" : "black";
      const node2 = {
        id: `node-${this.nodeId++}`,
        fen,
        move,
        step: this.currentStep,
        color,
        parentID: this.currentNode.id,
        children: [],
        comments: [],
        isCheckmate: isMoveCheckmate(move)
      };
      this.nodeMap.set(node2.id, node2);
      return node2;
    }
    peek() {
      return this.tokens[this.currentIndex];
    }
    consume() {
      return this.tokens[this.currentIndex++];
    }
    match(type) {
      return this.peek().type === type;
    }
    processMove(token, tokenType) {
      const tok = this.tokens[this.currentIndex - 1];
      const fen = this.currentNode.fen;
      this.chess.load(fen);
      try {
        const move = parseMoveInGame(this.chess, token, tokenType);
        if (!move) {
          if (this.strict) throw new Error(`Invalid move: ${token}`);
          this.skippedTokens.push({
            text: token,
            line: tok.line,
            column: tok.column,
            kind: "move"
          });
          this.parseAborted = true;
          return;
        }
        const newNode = this.createNode(move, this.chess.fen());
        this.currentNode.children.push(newNode);
        this.currentNode = newNode;
        this.currentStep++;
      } catch (e) {
        if (this.strict) throw e;
        this.skippedTokens.push({
          text: token,
          line: tok.line,
          column: tok.column,
          kind: "move"
        });
        this.parseAborted = true;
      }
    }
    parseVariation() {
      this.consume();
      const variationParentID = this.currentNode.parentID;
      if (!variationParentID) {
        while (!this.match("right-paren") && !this.match("eof") && !this.parseAborted) {
          this.consume();
        }
        if (this.match("right-paren")) this.consume();
        return;
      }
      const variationBase = this.nodeMap.get(variationParentID);
      const prevState = {
        node: this.currentNode,
        step: this.currentStep
      };
      this.currentNode = variationBase;
      this.currentStep = variationBase.step;
      while (!this.match("right-paren") && !this.match("eof") && !this.parseAborted) {
        if (this.isMoveToken()) {
          this.processMove(
            this.consume().value,
            this.tokens[this.currentIndex - 1].type
          );
        } else if (this.match("comment")) {
          this.parseComment();
        } else if (this.match("nag")) {
          this.parseNag();
        } else if (this.match("left-paren")) {
          this.parseVariation();
        } else if (this.match("result")) {
          const token = this.consume();
          const validResults = ["1-0", "0-1", "1/2-1/2"];
          if (validResults.includes(token.value)) {
            this.currentNode.result = token.value;
          }
          break;
        } else if (this.match("unknown")) {
          this.parseUnknown();
        } else {
          this.consume();
        }
      }
      if (this.match("right-paren")) {
        this.consume();
      }
      this.currentNode = prevState.node;
      this.currentStep = prevState.step;
    }
    parseComment() {
      const token = this.consume();
      const raw = token.value.replace(/^{|}$/g, "").replace(/^;/, "").trim();
      const evalMatch = raw.match(EVAL_REGEX);
      if (evalMatch) {
        const evalStr = evalMatch[1];
        const bestmove = evalMatch[2] || void 0;
        const ponder = evalMatch[3] || void 0;
        const glyphSymbol = evalMatch[4] || void 0;
        let scoreType = "cp";
        let score;
        if (evalStr.startsWith("m")) {
          scoreType = "mate";
          const mateStr = evalStr.slice(1);
          const isNeg = mateStr.startsWith("-");
          const mateVal = Number.parseInt(mateStr.replace(/[^0-9]/g, ""));
          score = isNeg ? -mateVal : mateVal;
        } else {
          score = Math.round(Number.parseFloat(evalStr) * 100);
        }
        this.currentNode.eval = { score, scoreType, depth: 0, bestmove, ponder };
        if (glyphSymbol) {
          const glyphDef = GLYPH_DEFS[glyphSymbol];
          if (glyphDef) this.currentNode.glyph = glyphDef;
        }
        return;
      }
      let annotPayload = null;
      if (raw.startsWith(`[${ANNOTATION_PREFIX}`) && raw.endsWith("]")) {
        annotPayload = raw.slice(ANNOTATION_PREFIX.length + 1, -1);
      } else if (raw.startsWith(ANNOTATION_PREFIX)) {
        annotPayload = raw.slice(ANNOTATION_PREFIX.length);
      }
      if (annotPayload !== null) {
        if (isAnnotationKey(annotPayload)) {
          pushAnnotation(this.currentNode, annotPayload);
          return;
        }
        const glyphDef = GLYPH_BY_SYMBOL[annotPayload];
        if (glyphDef) {
          if (glyphDef.category === "move") {
            this.currentNode.glyph = GLYPH_DEFS[glyphDef.symbol];
          } else {
            pushAnnotation(this.currentNode, glyphDef.key);
          }
          return;
        }
      }
      if (raw.startsWith(SHAPES_PREFIX)) {
        const shapesStr = raw.slice(SHAPES_PREFIX.length);
        const shapes2 = [];
        for (const part of shapesStr.split(",")) {
          const m = part.match(SHAPE_PART_REGEX);
          if (m) shapes2.push({ orig: m[1], dest: m[2], brush: m[3] });
        }
        if (shapes2.length > 0) {
          this.currentNode.shapes = shapes2;
          return;
        }
      }
      const {
        shapes,
        eval: evalMeta,
        clock,
        annotator,
        text
      } = extractCommentMeta(raw);
      if (shapes.length > 0) {
        (this.currentNode.shapes ??= []).push(...shapes);
      }
      if (evalMeta) {
        this.currentNode.eval = { ...evalMeta, depth: 0 };
      }
      if (clock) {
        this.currentNode.clock = clock;
      }
      if (text) {
        this.currentNode.comments ??= [];
        this.currentNode.comments.push(text);
        if (annotator) {
          (this.currentNode.commentAuthors ??= [])[this.currentNode.comments.length - 1] = annotator;
        }
      }
    }
    parseNag() {
      const token = this.consume();
      if (!this.currentNode.move) return;
      const glyphDef = nagToGlyph(token.value);
      if (glyphDef) {
        this.currentNode.glyph ??= glyphDef;
        return;
      }
      const annotationKey = nagToAnnotationKey(token.value);
      if (annotationKey) {
        pushAnnotation(this.currentNode, annotationKey);
      }
    }
    parseResult() {
      const token = this.consume();
      const validResults = ["1-0", "0-1", "1/2-1/2", "*"];
      if (validResults.includes(token.value)) {
        if (token.value !== "*") {
          this.currentNode.result = token.value;
        }
        this.tags.set("Result", token.value);
      }
    }
    getTags() {
      const lines = [];
      for (const [key, value] of this.tags.entries()) {
        lines.push(`[${key} "${value}"]`);
      }
      const fenIndex = lines.findIndex((line) => /^\[FEN "/.test(line));
      if (fenIndex !== -1 && !lines.some((line) => /^\[SetUp "/.test(line))) {
        lines.splice(fenIndex + 1, 0, `[SetUp "1"]`);
      }
      if (fenIndex !== -1 && !lines.some((line) => /^\[Variant "/.test(line)) && getPgnVariantTag(this.rootNode.fen)) {
        lines.splice(fenIndex, 0, `[Variant "Chess960"]`);
      }
      return lines.join("\n");
    }
    getRoot() {
      return this.rootNode;
    }
    getMap() {
      return this.nodeMap;
    }
    getSkipped() {
      return this.skippedTokens;
    }
    getMainLine() {
      const mainLine = [];
      let current = this.rootNode;
      while (current.children.length > 0) {
        const mainChild = current.children[0];
        mainLine.push(mainChild);
        current = mainChild;
      }
      return mainLine;
    }
  };

  // kernel/upstream/utils/parse.ts
  function splitPGN(pgn2) {
    const lines = pgn2.split("\n");
    const games = [];
    let currentLines = [];
    let hasMovetext = false;
    let hasTags = false;
    let commentDepth = 0;
    let blankAfterTags = false;
    for (const line of lines) {
      const trimmed = line.trim();
      for (let i = 0; i < trimmed.length; i++) {
        if (trimmed[i] === "{") commentDepth++;
        else if (trimmed[i] === "}") commentDepth = Math.max(0, commentDepth - 1);
      }
      const inComment = commentDepth > 0;
      const isTagLine = /^\[\w+\s+"[^"]*"\]$/.test(trimmed);
      if (isTagLine && !inComment) {
        if (hasMovetext && currentLines.length > 0) {
          games.push(makeSlot(currentLines));
          currentLines = [];
          hasMovetext = false;
          hasTags = false;
          blankAfterTags = false;
        } else if (blankAfterTags && hasTags) {
          games.push(makeSlot(currentLines));
          currentLines = [];
          hasMovetext = false;
          hasTags = false;
          blankAfterTags = false;
        }
        currentLines.push(line);
        hasTags = true;
        blankAfterTags = false;
      } else if (trimmed === "") {
        if (hasTags && !hasMovetext) {
          blankAfterTags = true;
        }
        currentLines.push(line);
      } else if (!inComment) {
        hasMovetext = true;
        blankAfterTags = false;
        currentLines.push(line);
      } else {
        currentLines.push(line);
      }
    }
    if (currentLines.some((l) => l.trim() !== "")) {
      games.push(makeSlot(currentLines));
    }
    if (games.length === 0) {
      games.push(makeSlot([""]));
    }
    return games;
  }
  function makeSlot(lines) {
    const raw = lines.join("\n");
    const headers = extractHeaders(raw);
    return { raw, headers };
  }
  function extractHeaders(raw) {
    const headers = /* @__PURE__ */ new Map();
    const tagRe = /^\[(\w+)\s+"([^"]*)"\]$/gm;
    let m;
    while ((m = tagRe.exec(raw)) !== null) {
      headers.set(m[1], m[2]);
    }
    return headers;
  }

  // kernel/upstream/utils/stringify-pgn.ts
  function getFullmoveFromFen(fen) {
    const n = Number.parseInt(fen.split(" ")[5] ?? "", 10);
    return Number.isNaN(n) || n < 1 ? 1 : n;
  }
  function genNodeBrothers(root) {
    const map = /* @__PURE__ */ new Map();
    function dfs(node2) {
      if (node2.children.length > 1) {
        const [main, ...siblings] = node2.children;
        map.set(main, siblings);
      }
      for (const child of node2.children) dfs(child);
    }
    dfs(root);
    return map;
  }
  var COLOR_BY_BRUSH = {
    g: "G",
    r: "R",
    y: "Y",
    b: "B"
  };
  function shapeColor(brush) {
    return COLOR_BY_BRUSH[brush] ?? "B";
  }
  function formatLichessEval(ev) {
    if (ev.scoreType === "mate") {
      return `#${ev.score >= 0 ? "" : "-"}${Math.abs(ev.score)}`;
    }
    const pawns = ev.score / 100;
    return `${pawns >= 0 ? "+" : "-"}${Math.abs(pawns).toFixed(2)}`;
  }
  function formatPrivateEval(ev) {
    const absScore = Math.abs(ev.score);
    return ev.scoreType === "mate" ? `m${ev.score >= 0 ? "+" : "-"}${absScore}` : `${ev.score >= 0 ? "+" : "-"}${(absScore / 100).toFixed(2)}`;
  }
  function serializeNodeMeta(node2, options = {}) {
    const includeComments = options.includeComments ?? true;
    const includeEval = options.includeEval ?? true;
    const annotationFormat = options.annotationFormat ?? "suffix";
    let meta = "";
    if (node2.glyph) {
      if (annotationFormat === "nag") {
        const nag = GLYPH_BY_SYMBOL[node2.glyph.symbol]?.nag;
        meta += nag ? ` $${nag}` : node2.glyph.symbol;
      } else if (annotationFormat === "comment") {
        meta += ` {[${ANNOTATION_PREFIX}${node2.glyph.symbol}]}`;
      } else {
        meta += node2.glyph.symbol;
      }
    }
    if (node2.annotations?.length) {
      for (const key of node2.annotations) {
        if (/^\d+$/.test(key)) {
          if (annotationFormat === "comment") {
            const def = GLYPH_BY_KEY[key];
            meta += def ? ` {[${ANNOTATION_PREFIX}${def.symbol}]}` : ` $${key}`;
          } else {
            meta += ` $${key}`;
          }
        } else {
          meta += ` {${ANNOTATION_PREFIX}${key}}`;
        }
      }
    }
    if (includeComments && node2.comments?.length) {
      for (let i = 0; i < node2.comments.length; i++) {
        const c = node2.comments[i];
        const author = node2.commentAuthors?.[i];
        meta += author ? ` {[%anno "${author.name}"${author.user ? `, ${author.user}` : ""}] ${c}}` : ` {${c}}`;
      }
    }
    if (includeComments && node2.clock) {
      meta += ` {[%clk ${node2.clock}]}`;
    }
    if (node2.shapes?.length) {
      const highlights = node2.shapes.filter((s) => !s.dest).map((s) => shapeColor(s.brush) + s.orig);
      const arrows = node2.shapes.filter((s) => s.dest).map((s) => shapeColor(s.brush) + s.orig + s.dest);
      let block = "";
      if (highlights.length) block += `[%csl ${highlights.join(",")}]`;
      if (arrows.length) block += `[%cal ${arrows.join(",")}]`;
      if (block) meta += ` { ${block} }`;
    }
    if (includeEval && node2.eval) {
      meta += ` { [%eval ${formatLichessEval(node2.eval)}] }`;
      if (node2.eval.bestmove) {
        let annotation = `%e:${formatPrivateEval(node2.eval)}`;
        annotation += `,${node2.eval.bestmove}`;
        if (node2.eval.ponder) annotation += `,${node2.eval.ponder}`;
        meta += ` {${annotation}}`;
      }
    }
    return meta;
  }
  function stringifyPGN(root, includeEval = true, annotationFormat = "suffix") {
    const nodeBrothers = genNodeBrothers(root);
    const fullmove = getFullmoveFromFen(root.fen);
    const rootStep = getTurnFromFen(root.fen) === "black" ? fullmove : fullmove - 1;
    function walk(node2, stepNum, lineStart = false, inVariation = false) {
      let result = "";
      if (node2.move) {
        const notation = getSaveNotation(node2.move);
        if (node2.color === "white") {
          result += `${stepNum}. ${notation}`;
        } else if (node2.color === "black") {
          result += lineStart ? `${stepNum}... ${notation}` : notation;
        }
      }
      const meta = serializeNodeMeta(node2, { includeEval, annotationFormat });
      result += meta;
      const brothers = nodeBrothers.get(node2);
      if (brothers?.length) {
        for (const brother of brothers) {
          result += ` (${walk(brother, stepNum, brother.color === "black", true)})`;
        }
      }
      if (node2.children[0]) {
        const next = node2.children[0];
        const nextStepNum = next.color === "white" ? stepNum + 1 : stepNum;
        const blackStarts = next.color === "black" && (node2 === root || meta.includes("{") || Boolean(brothers?.length));
        result += ` ${walk(next, nextStepNum, blackStarts, inVariation)}`;
      } else if (node2.result) {
        result += ` ${node2.result}`;
      } else if (!inVariation) {
        result += " *";
      }
      return result;
    }
    return walk(root, rootStep).trim();
  }

  // kernel/bridge.ts
  var metaKeys = [
    "eval",
    "glyph",
    "annotations",
    "shapes",
    "clock",
    "commentAuthors",
    "result"
  ];
  var sq = (s) => s.charCodeAt(0) - 97 + Number(s[1]) * 9;
  var algebraic2 = (s) => String.fromCharCode(97 + s % 9) + Math.floor(s / 9);
  function metadata(n) {
    const result = {};
    for (const k of metaKeys) if (n[k] !== void 0) result[k] = n[k];
    return result;
  }
  function flatten(root) {
    const result = [];
    const stack = [root];
    while (stack.length) {
      const n = stack.pop();
      result.push({
        id: n.id,
        x: n.x ?? 0,
        y: n.y ?? 0,
        parent: n.parentID ?? null,
        children: n.children.map((x) => x.id),
        fen: n.fen,
        move: n.move ? {
          from: sq(n.move.from),
          to: sq(n.move.to),
          zh: n.move.zh,
          iccs: n.move.iccs,
          lan: n.move.lan
        } : null,
        comments: n.comments ?? [],
        meta: metadata(n)
      });
      for (let i = n.children.length - 1; i >= 0; i--) stack.push(n.children[i]);
    }
    return result;
  }
  function hydrate(data) {
    const byId = new Map(data.nodes.map((n) => [n.id, n]));
    const all = /* @__PURE__ */ new Map();
    const rootData = byId.get(data.root);
    if (!rootData) throw Error("Missing root");
    const root = {
      id: rootData.id,
      parentID: null,
      fen: new Chess(data.fen).fen(),
      move: null,
      children: [],
      comments: rootData.comments,
      ...rootData.meta
    };
    all.set(root.id, root);
    const stack = [root];
    while (stack.length) {
      const n = stack.pop();
      const source = byId.get(n.id);
      for (const id of source.children) {
        const child = byId.get(id);
        if (!child?.move) throw Error("Missing child move");
        const chess = new Chess(n.fen);
        const m = chess.move(
          algebraic2(child.move.from) + algebraic2(child.move.to)
        );
        if (!m) throw Error("Illegal stored move");
        const node2 = {
          id,
          parentID: n.id,
          fen: chess.fen(),
          move: m,
          color: m.color === "w" ? "white" : "black",
          children: [],
          comments: child.comments,
          ...child.meta
        };
        n.children.push(node2);
        all.set(id, node2);
        stack.push(node2);
      }
    }
    return { root, all };
  }
  function dispatch(request) {
    const { op, data = {} } = JSON.parse(request);
    let result;
    switch (op) {
      case "initial":
        result = { fen: new Chess(DEFAULT_FEN).fen() };
        break;
      case "position": {
        const c = new Chess(data.fen);
        result = {
          fen: c.fen(),
          board: c.board(),
          turn: c.turn(),
          check: c.isCheck(),
          checkmate: c.isCheckmate(),
          stalemate: c.isStalemate(),
          moves: c.moves({ verbose: true }).map((m) => ({
            from: sq(m.from),
            to: sq(m.to),
            zh: m.zh,
            iccs: m.iccs,
            lan: m.lan
          }))
        };
        break;
      }
      case "move": {
        const c = new Chess(data.fen);
        const m = c.move(
          data.notation ?? algebraic2(data.from) + algebraic2(data.to)
        );
        result = {
          fen: c.fen(),
          move: {
            from: sq(m.from),
            to: sq(m.to),
            zh: m.zh,
            iccs: m.iccs,
            lan: m.lan
          },
          check: c.isCheck(),
          result: c.isCheckmate() || c.isStalemate() ? c.turn() === "b" ? "1-0" : "0-1" : null
        };
        break;
      }
      case "parse":
        result = splitPGN(data.pgn).map((slot) => {
          const p = new PGNParser(slot.raw);
          return {
            headers: Object.fromEntries(p.tags),
            fen: p.getRoot().fen,
            root: p.getRoot().id,
            nodes: flatten(p.getRoot()),
            warnings: p.getSkipped()
          };
        });
        break;
      case "layout": {
        const h = hydrate(data);
        h.all.set("node-root", h.root);
        calculateTreeLayout(h.all, /* @__PURE__ */ new Set(), 1);
        result = { nodes: flatten(h.root) };
        break;
      }
      case "hydrate": {
        const h = hydrate(data);
        result = { nodes: flatten(h.root) };
        break;
      }
      case "stringify": {
        const h = hydrate(data);
        const tags = Object.entries(data.headers ?? {}).map(
          ([k, v]) => "[" + k + ' "' + String(v).replace(/\\/g, "\\\\").replace(/"/g, '\\"') + '"]'
        ).join("\n");
        result = tags + "\n\n" + stringifyPGN(h.root, true, data.annotationFormat ?? "suffix");
        break;
      }
      default:
        throw Error("Unknown operation: " + op);
    }
    return JSON.stringify(result);
  }
  globalThis.XiangqiKernel = { dispatch };
})();
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
