// C++ port of obsidian-xiangqi's Walker/Buchheim tree layout.
#include "rules.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <unordered_map>
namespace xqn::native {
namespace {
struct W {
    size_t index{};
    int depth{};
    double x{}, y{}, rel{}, prelim{}, shift{}, change{}, lr{}, rr{};
    W *left = this, *right = this, *lt{}, *rt{};
    std::vector<W *> kids;
};
struct Low {
    double y;
    int index;
    Low *next;
};
double bottom(W *w) { return w->y + (w->kids.empty() ? 2 : 1); }
W *nextleft(W *w) { return !w->kids.empty() && w->kids.front() != w->lt ? w->kids.front() : w->lt; }
W *nextright(W *w) { return !w->kids.empty() && w->kids.back() != w->rt ? w->kids.back() : w->rt; }
void separate(W *w, int i, Low *lows) {
    auto prev = w->kids[i - 1], cur = w->kids[i];
    W *r = prev, *l = cur;
    double rs = prev->rel, ls = cur->rel;
    bool first = true;
    while (r && l) {
        if (bottom(r) > lows->y && lows->next)
            lows = lows->next;
        double dist = rs + r->prelim - ls - l->prelim + 2;
        if (dist > 0 || (dist < 0 && first)) {
            ls += dist;
            cur->rel += dist;
            cur->lr += dist;
            cur->rr += dist;
            int n = i - lows->index;
            if (n > 1) {
                double delta = dist / n;
                w->kids[lows->index + 1]->shift += delta;
                cur->shift -= delta;
                cur->change -= dist - delta;
            }
        }
        first = false;
        double rb = bottom(r), lb = bottom(l);
        if (rb <= lb) {
            r = nextright(r);
            if (r)
                rs += r->rel;
        }
        if (rb >= lb) {
            l = nextleft(l);
            if (l)
                ls += l->rel;
        }
    }
    if (!r && l) {
        auto f = w->kids.front(), ext = f->left;
        ext->lt = l;
        double diff = ls - l->rel - f->lr;
        ext->rel += diff;
        ext->prelim -= diff;
        f->left = cur->left;
        f->lr = cur->lr;
    } else if (r && !l) {
        auto ext = cur->right;
        ext->rt = r;
        double diff = rs - r->rel - cur->rr;
        ext->rel += diff;
        ext->prelim -= diff;
        cur->right = prev->right;
        cur->rr = prev->rr;
    }
}
void arrange(W *w) {
    w->y = w->depth;
    if (w->depth % 2 == 0)
        std::reverse(w->kids.begin(), w->kids.end());
    std::vector<std::unique_ptr<Low>> storage;
    Low *lows = nullptr;
    for (int i = 0; i < int(w->kids.size()); i++) {
        auto kid = w->kids[i];

        double lowY = bottom(i == 0 ? kid->left : kid->right);
        if (i)
            separate(w, i, lows);
        while (lows && lowY >= lows->y)
            lows = lows->next;
        storage.push_back(std::make_unique<Low>(Low{lowY, i, lows}));
        lows = storage.back().get();
    }
    double lastShift = 0, lastChange = 0;
    for (auto c : w->kids) {
        double s = lastShift + c->shift, change = lastChange + s + c->change;
        c->rel += change;
        lastShift = s;
        lastChange = change;
    }
    if (w->kids.empty())
        return;
    bool even = w->depth % 2 == 0;
    auto align = even ? w->kids.back() : w->kids.front(), last = w->kids.back();
    w->prelim = align->prelim + align->rel;
    w->left = even ? w->kids.front()->left : align->left;
    w->lr = even ? w->kids.front()->lr : align->lr;
    w->right = last->right;
    w->rr = last->rr;
}
void resolve(W *root, Json &nodes) {
    struct Pending {
        W *node;
        double sum;
    };
    std::vector<Pending> pending = {{root, -root->rel - root->prelim}};
    while (!pending.empty()) {
        auto entry = pending.back();
        pending.pop_back();
        auto w = entry.node;
        double sum = entry.sum + w->rel;
        w->x = sum + w->prelim;
        nodes[w->index]["x"] = std::floor(w->x * 10 + .5) / 10;
        nodes[w->index]["y"] = w->depth;
        for (auto child : w->kids)
            pending.push_back({child, sum});
    }
}

} // namespace
void layout(Json &nodes) {
    if (nodes.empty())
        return;
    std::vector<std::unique_ptr<W>> all;
    std::unordered_map<std::string, W *> map;
    for (size_t i = 0; i < nodes.size(); i++) {
        auto w = std::make_unique<W>();
        w->index = i;
        map.emplace(nodes[i].at("id").get<std::string>(), w.get());
        all.push_back(std::move(w));
    }
    for (auto &w : all)
        for (auto &id : nodes[w->index].at("children")) {
            auto c = map.at(id.get<std::string>());
            c->depth = w->depth + 1;
            if (c->depth > 2048)
                throw std::runtime_error("Tree exceeds layout depth limit");
            w->kids.push_back(c);
        }
    auto root = all.front().get();
    for (auto i = all.rbegin(); i != all.rend(); ++i)
        arrange(i->get());
    resolve(root, nodes);
}
} // namespace xqn::native
