#ifndef __WAVELET_TRIE__
#define __WAVELET_TRIE__

#include<iostream>
#include<fstream>
#include<sdsl/wavelet_trees.hpp>
#include<stack>
#include<vector>
#include<tuple>
#include<unordered_map>
#include<unordered_set>
#include<boost/functional/hash.hpp>
#include<map>
#include<omp.h>
#include<algorithm>
#include<parallel/algorithm>
//#include "array_int.hpp"
#define MAXNUM (std::numeric_limits<std::uint16_t>::max())
#define TASKMIN 100

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/gmp.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/unordered_set.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include<functional>
#include<type_traits>


namespace WaveletTrie {
    typedef boost::multiprecision::mpz_int cpp_int;
    typedef bool (*bittest_t)(const cpp_int&, uint32_t i);
    bittest_t bit_test = boost::multiprecision::bit_test;
    /*
    typedef cpp_int& (*bitset_t)(cpp_int&, uint32_t i);
    bitset_t bit_set = boost::multiprecision::bit_set;
    bitset_t bit_unset = boost::multiprecision::bit_unset;
    typedef uint32_t (*sigbit_t)(const cpp_int&);
    sigbit_t msb = boost::multiprecision::msb;
    sigbit_t lsb = boost::multiprecision::lsb;
    */

    //typedef array_int::array_int annot_t;
    typedef cpp_int annot_t;
    typedef std::vector<annot_t> intvec_t;
    typedef sdsl::sd_vector<> beta_t;
    //typedef sdsl::rrr_vector<> beta_t;
    typedef beta_t::rank_1_type rank1_t;
    typedef beta_t::rank_0_type rank0_t;
    //typedef array_int::array_int alpha_t;
    typedef cpp_int alpha_t;
    //typedef sdsl::sd_vector<> alpha_t;
    typedef sdsl::bit_vector bv_t;
    //common prefix, allequal, size of block
    typedef std::tuple<alpha_t, bool> prefix_t;
    typedef std::tuple<alpha_t, bool, intvec_t> prefix_trans_t;
    typedef boost::archive::binary_oarchive oarchive_t;
    typedef boost::archive::binary_iarchive iarchive_t;

    /*
    void clear_after(array_int::array_int &a, size_t i) {
        a.indices.erase(a.indices.lower_bound(i), a.indices.end());
    }
    */

    void clear_after(cpp_int &a, size_t i) {
        mpz_t& cur = a.backend().data();
        mpz_tdiv_r_2exp(cur, cur, i);

        //a &= temp;
//        if (i == 0 || a.backend().size() == 0) {
//            a = 0;
//            return;
//        }
//        size_t limb_size=sizeof(*a.backend().limbs())*8lu;
//        size_t start = ceil((double)i/(double)limb_size);
//        if (start*limb_size > i) {
//            if (start < a.backend().size() + 1)
//                *(a.backend().limbs()+start-1) &= (1lu << (limb_size + i + 1 - start*limb_size))-1;
//        }
//        if (start < a.backend().size()) {
//            std::fill(a.backend().limbs()+start, a.backend().limbs()+a.backend().size(), 0lu);
//        }
    }

    class Node {
        public:
            Node(Node *that, bool copy=true);
            Node();
            void serialize(std::ofstream &os, std::ofstream &oas);
            void print(std::ostream &os, bool recursive=false);
            void print(bool recursive=false);
            ~Node();
            size_t add(intvec_t *iv, size_t begin, size_t end, prefix_t prefix);
            void append(Node *other, bool leftorright=false, size_t addme=0, size_t addother=0);
            alpha_t alpha=1;
            beta_t beta;
            rank1_t rank1;
            rank0_t rank0;
            Node *child[2] = {NULL, NULL};
            static bool process_node(std::tuple<Node*, intvec_t*, prefix_t, size_t> &curnode);
            int move_label_down(size_t ol, size_t add=0);
            void breakleaf(size_t addme);
            static Node* merge(std::vector<Node*>::iterator begin, std::vector<Node*>::iterator end);
            void set_beta(const beta_t &beta) {
                this->beta = beta;
                sdsl::util::init_support(this->rank1, &(this->beta));
                sdsl::util::init_support(this->rank0, &(this->beta));
            }
            void set_beta(beta_t&& beta) {
                this->beta = beta;
                sdsl::util::init_support(this->rank1, &(this->beta));
                sdsl::util::init_support(this->rank0, &(this->beta));
            }
    };
    typedef std::tuple<Node*, intvec_t*, prefix_t, size_t> nodestate_t;

    Node::Node() {
    }

    Node::Node(Node *that, bool copy) {
        this->alpha = that->alpha;
        this->beta = that->beta;
        this->rank1 = that->rank1;
        this->rank0 = that->rank0;
        if (that->child[0]) {
            if (copy) {
                this->child[0] = new Node(that->child[0]);
            } else {
                this->child[0] = that->child[0];
                that->child[0] = NULL;
            }
        }
        if (that->child[1]) {
            if (copy) {
                this->child[1] = new Node(that->child[1]);
            } else {
                this->child[1] = that->child[1];
                that->child[1] = NULL;
            }
        }
    }

    //bv_t copy_bits(array_int::array_int &abv, size_t length=0) {
    //    return bv_t();
    //}

    bv_t copy_bits(const cpp_int &cbv, size_t length=0) {
        if (cbv == 0) {
            return bv_t(1);
        }
        /*
        size_t limb_size = sizeof(*cbv.backend().limbs())*8lu;
        bv_t bv(cbv.backend().size()*limb_size);
        assert(cbv.backend().size() <= (bv.capacity()>>6));
        std::copy(cbv.backend().limbs(), cbv.backend().limbs()+cbv.backend().size(), bv.data());
        bv.resize(length == 0 ? msb(cbv)+1 : length);
        */
        const mpz_t& data = cbv.backend().data();
        size_t size_bytes = (mpz_sizeinbase (data, 2) + CHAR_BIT-1) / CHAR_BIT;
        bv_t bv(size_bytes * 8);
        mpz_export(bv.data(), &size_bytes, 1, 1, 0, 0, data);
        return bv;
    }

    /*
    array_int::array_int copy_bits(bv_t &sbv) {
        return array_int::array_int();
    }
    cpp_int copy_bits(bv_t &sbv) {
        cpp_int cbv=0;
        bit_set(cbv, sbv.size());
        assert((sbv.capacity()>>6) <= cbv.backend().size());
        std::copy(sbv.data(), sbv.data()+(sbv.capacity()>>6), cbv.backend().limbs());
        bit_unset(cbv, sbv.size());
        return cbv;
    }
    */

    void copy_bits(bv_t &target, beta_t &source, size_t start=0, size_t t_bs=15) {
        assert(target.size()-start >= source.size());
        //bv_t test = target;
        size_t i=0;
        for (;i+t_bs<source.size();i+=t_bs) {
            target.set_int(start+i, source.get_int(i, t_bs), t_bs);
        }
        target.set_int(start+i, source.get_int(i, source.size()-i), source.size()-i);
        /*
        //sanity check
        for (size_t i=0;i<source.size();++i)
            test[start+i]=source[i];
        if (test != target) {
            std::cerr << "start: " << start << "\tb_start: " << b_start << "\ttotalsize: " << totalsize << "\n";
            std::cerr << "other: " << source << "\n";
            std::cerr << "E: " << test << "\tO: " << target << "\n";
            assert(false);
        }
        */
    }

    void Node::serialize(std::ofstream &os, std::ofstream &oas) {
        //oarchive_t oa(os);
        //oarchive_t oaa(oas);
        //bv_t bv = copy_bits(this->alpha);
        bv_t bv = copy_bits(this->alpha);
        //bv_t bv(boost::multiprecision::msb(this->alpha));
        //for (size_t i=0;i<bv.size();++i) {
        //    bv[i] = boost::multiprecision::bit_test(this->alpha, i);
        //}
        //sdsl::sd_vector<> sd(bv);
        beta_t sd(bv);
        //oa & this->alpha;
        sd.serialize(os);
        this->beta.serialize(oas);
        this->rank1.serialize(oas);
        this->rank0.serialize(oas);
    }

    class WTR {
        public:
            WTR();
            WTR(intvec_t *iv, size_t osize, size_t batch);
            ~WTR();
            annot_t at(size_t i);
            void serialize(std::ofstream &os);
            void serialize_concat(std::ofstream &os, std::ofstream &oas);
            void load(std::ifstream &is);
            bool reconstruct(intvec_t::iterator ivb, intvec_t::iterator ive);
            bool reconstruct(intvec_t &iv);
            void print(std::ostream &os);
            void print();
            void append(WTR &other);
            static WTR merge(std::vector<WTR*>::iterator begin, std::vector<WTR*>::iterator end);
        private:
            size_t height=0;
            size_t size=0;
            size_t maxannot=0;
            Node *root=NULL;
            void check_structure();
    };

    void WTR::append(WTR &other) {
        this->size += other.size;
        #pragma omp parallel
        #pragma omp single nowait
        this->root->append(other.root, 0, this->size, other.size);
    }

    void WTR::serialize(std::ofstream &os) {
        std::stack<Node*> node_stack;
        if (this->root != NULL)
            node_stack.push(this->root);
        Node *cur;
        while (node_stack.size()) {
            cur = node_stack.top();
            node_stack.pop();
            cur->serialize(os, os);
            if (cur->child[0] != NULL)
                node_stack.push(cur->child[0]);
            if (cur->child[1] != NULL)
                node_stack.push(cur->child[1]);
        }
    }

    void WTR::serialize_concat(std::ofstream &os, std::ofstream &oas) {
        //oarchive_t oa(os);
        std::stack<Node*> node_stack;
        std::vector<alpha_t> alphas;
        std::vector<beta_t> betas;
        std::vector<size_t> alpha_l;
        std::vector<size_t> beta_l;
        //std::vector<rank1_t> rank1s;
        //std::vector<rank0_t> rank0s;
        if (this->root != NULL)
            node_stack.push(this->root);
        Node *cur;
        size_t asize=0;
        size_t bsize=0;
        while (node_stack.size()) {
            cur = node_stack.top();
            node_stack.pop();
            alphas.push_back(cur->alpha);
            betas.push_back(cur->beta);
            alpha_l.push_back(msb(cur->alpha));
            beta_l.push_back(cur->beta.size());
            asize += alpha_l.back();
            bsize += beta_l.back();
            //rank1s.push_back(cur->rank1);
            //rank0s.push_back(cur->rank0);
            if (cur->child[0] != NULL)
                node_stack.push(cur->child[0]);
            if (cur->child[1] != NULL)
                node_stack.push(cur->child[1]);
        }
        std::cout << asize << "\t" << bsize << "\n";
        bv_t bv_alpha(asize);
        bv_t bv_beta(bsize);
        size_t ai=0,bi=0;
        for (size_t i=0;i<alpha_l.size();++i) {
#ifdef DBGDEBUG
            assert(alpha_l[i] == msb(alphas[i]));
#endif
            for (size_t j=0;j<alpha_l[i];++j) {
                bv_alpha[ai++] = bit_test(alphas[i], j);
            }
#ifdef DBGDEBUG
            assert(beta_l[i] == betas[i].size());
#endif
            for (size_t j=0;j<beta_l[i];++j) {
                bv_beta[bi++] = betas[i][j];
            }
        }
        beta_t alpha = beta_t(bv_alpha);
        beta_t beta = beta_t(bv_beta);
        alpha.serialize(os);
        beta.serialize(oas);
    }

    Node::~Node() {
        if (this->child[0] != NULL) {
            delete this->child[0];
            this->child[0] = NULL;
        }
        if (this->child[1] != NULL) {
            delete this->child[1];
            this->child[1] = NULL;
        }
    }

    WTR::~WTR() {
        if (this->root != NULL) {
            delete this->root;
            this->root = NULL;
        }
    }

    void Node::breakleaf(size_t addme) {
        if (addme) {
            this->set_beta(beta_t(bv_t(addme)));
            //this->beta = beta_t(bv_t(addme));
            //sdsl::util::init_support(this->rank1, &(this->beta));
            //sdsl::util::init_support(this->rank0, &(this->beta));
#ifdef DBGDEBUG
            assert(this->child[0] == NULL);
            assert(this->child[1] == NULL);
#endif
            this->child[0] = new Node();
            this->child[0]->alpha = 1;
        }
    }


    prefix_t longest_prefix(intvec_t::iterator ivb, intvec_t::iterator ive, size_t begin=0, size_t end=MAXNUM) {
        std::pair<alpha_t, size_t> prefix(1,0);
        bool allequal=true;
        intvec_t::iterator i = ivb;
        mpz_t temp2;
        if ((ive - ivb) > 0) {
            //std::get<0>(prefix) = array_int::toint(*ivb);
            std::get<0>(prefix) = *ivb;
            std::get<1>(prefix) = MAXNUM;
            for (++i;i != ive;++i) {
                if (*i != std::get<0>(prefix)) {
                    mpz_init(temp2);
                    mpz_xor(temp2, i->backend().data(), std::get<0>(prefix).backend().data());
                    //std::get<1>(prefix) = std::min((size_t)std::get<1>(prefix), (size_t)lsb(((*i) ^ (alpha_t)std::get<0>(prefix))));
                    std::get<1>(prefix) = std::min((size_t)std::get<1>(prefix), mpz_scan1(temp2, 0));
                    clear_after(std::get<0>(prefix), std::get<1>(prefix));
                    if (std::get<1>(prefix) == 0) {
                        break;
                    }
                    mpz_clear(temp2);
                }
            }
            if (std::get<1>(prefix) == MAXNUM) {
                std::get<1>(prefix) = (std::get<0>(prefix) != 0) ? msb((alpha_t)std::get<0>(prefix))+1 : 0;
            } else {
                allequal = false;
            }
            //set the last bit to indicate the length of the prefix
            bit_set(std::get<0>(prefix), std::get<1>(prefix));
        }
        return std::make_tuple(std::get<0>(prefix), allequal);

    }

    //TODO: instead of assigning vectors, just sort
    //put a stable sort here
    void distribute_(nodestate_t &curnode, intvec_t** children, bv_t &bv, annot_t* prefices, bool* set, size_t* lengths, size_t &length) {
        size_t i = 0;
        bool curbit;
        mpz_t temp, temp2;
        for (auto it = bv.begin(); it != bv.end(); ++it) {
            mpz_t& old = std::get<1>(curnode)->operator[](i).backend().data();
            curbit = mpz_tstbit(old, length);
            mpz_t& pref = prefices[curbit].backend().data();
            *it = curbit;
            mpz_init(temp);
            mpz_tdiv_q_2exp(temp, old, length + 1);
            children[curbit]->emplace_back(temp);
            if (children[curbit]->back() != prefices[curbit]) {
                if (!set[curbit]) {
                    prefices[curbit] = children[curbit]->back();
                    set[curbit]=true;
                } else {
                    mpz_init(temp2);
                    mpz_xor(temp2, temp, pref);
                    lengths[curbit] = std::min(lengths[curbit], mpz_scan1(temp2, 0));
                    mpz_clear(temp2);
                    clear_after(prefices[curbit], lengths[curbit]);
                }
            }
            mpz_clear(temp);
            ++i;
        }
    }


    bool Node::process_node(nodestate_t &curnode) {
        while (std::get<1>(curnode)) {
            if (std::get<1>(curnode)->size()) {
                std::get<0>(curnode)->alpha = std::get<0>(std::get<2>(curnode));
                size_t length = mpz_sizeinbase(std::get<0>(curnode)->alpha.backend().data(), 2) - 1;
                //size_t length = msb(std::get<0>(curnode)->alpha);
                if (!std::get<1>(std::get<2>(curnode))) {
                    intvec_t* children[2] = {new intvec_t(), new intvec_t()};
                    std::get<0>(curnode)->child[0] = new Node();
                    std::get<0>(curnode)->child[1] = new Node();
                    bv_t bv(std::get<1>(curnode)->size());
                    annot_t prefices[2];
                    bit_set(prefices[0], MAXNUM);
                    bit_set(prefices[1], MAXNUM);
                    prefix_t cprefix[2] = {prefix_t(0, true), prefix_t(0, true)};
                    bool set[2] = {false, false};
                    size_t lengths[2] = {MAXNUM, MAXNUM};
                    distribute_(curnode, children, bv, prefices, set, lengths, length);
                    for (size_t i=0;i<2;++i) {
                        std::get<0>(cprefix[i]) = prefices[i];
                        if (lengths[i] == MAXNUM) {
                            lengths[i] = (std::get<0>(cprefix[i]) != 0) ? msb((alpha_t)std::get<0>(cprefix[i]))+1 : 0;
                        } else {
                            std::get<1>(cprefix[i]) = false;
                        }
                        //set the last bit to indicate the length of the prefix
                        bit_set(std::get<0>(cprefix[i]), lengths[i]);
                    }
                    std::get<0>(curnode)->set_beta(beta_t(bv));
#ifdef DBGDEBUG
                    assert(children[0]->size());
                    assert(children[1]->size());
#endif
                    delete std::get<1>(curnode);
                    nodestate_t leftnode = std::make_tuple(std::get<0>(curnode)->child[0], children[0], cprefix[0], std::get<3>(curnode));
                    nodestate_t rightnode = std::make_tuple(std::get<0>(curnode)->child[1], children[1], cprefix[1], std::get<3>(curnode));
                    if (lengths[0] > lengths[1]) {
                        #pragma omp task if (children[0]->size() > TASKMIN)
                        process_node(leftnode);
                        curnode = rightnode;
                    } else {
                        #pragma omp task if (children[1]->size() > TASKMIN)
                        process_node(rightnode);
                        curnode = leftnode;
                    }
                } else {
                    std::get<0>(curnode)->breakleaf(std::get<1>(curnode)->size());
                    delete std::get<1>(curnode);
                    std::get<1>(curnode) = NULL;
                    return true;
                }
            } else {
                std::get<0>(curnode)->breakleaf(std::get<1>(curnode)->size());
                delete std::get<1>(curnode);
                std::get<1>(curnode) = NULL;
                return true;
            }
        }
        return true;
    }

    //TODO: replace process_node(start) with process_node(begin, end)
    size_t Node::add(intvec_t *iv, size_t begin, size_t end, prefix_t prefix) {
        nodestate_t start = std::make_tuple(this, iv, prefix, 0);
        #pragma omp task
        Node::process_node(start);
        return 0;
    }

    int Node::move_label_down(size_t ol, size_t add) {
        size_t len = msb(this->alpha);
        if (ol > len) {
            this->alpha = 0;
            bit_set(this->alpha, ol);
            //this->alpha = alpha_t(1) << ol;
            return 3;
        }
        if (ol == len)
            return 1;
        Node *temp_node = new Node();
        mpz_t& temp = temp_node->alpha.backend().data();
        mpz_tdiv_q_2exp(temp, this->alpha.backend().data(), ol + 1);
        //temp_node->alpha = this->alpha >> (ol+1);
        temp_node->beta = this->beta;
        temp_node->child[0] = this->child[0];
        temp_node->child[1] = this->child[1];
        this->set_beta(beta_t(bv_t(this->beta.size() == 0 ? add : this->beta.size(), bit_test(this->alpha, ol))));
        //this->beta = beta_t(bv_t(this->beta.size() == 0 ? add : this->beta.size(), bit_test(this->alpha, ol)));
        //sdsl::util::init_support(this->rank1, &(this->beta));
        //sdsl::util::init_support(this->rank0, &(this->beta));
        //this->alpha = (this->alpha & ((alpha_t(1) << (ol))-1)) | (alpha_t(1) << ol);
        clear_after(this->alpha, ol);
        bit_set(this->alpha, ol);
        if (this->beta[0]) {
            this->child[1] = temp_node;
            this->child[0] = NULL;
        } else {
            this->child[0] = temp_node;
            this->child[1] = NULL;
        }
#ifdef DBGDEBUG
        assert(msb(this->alpha) == ol);
#endif
        return 0;
    }

    void Node::append(Node *other, bool leftorright, size_t addme, size_t addother) {
#ifdef DBGDEBUG
        this->print(true);
        std::cout << "-\n";
#endif
        if (other == NULL) {
#ifdef DBGDEBUG
            std::cout << "-\n";
#endif
            //propagate on the right side
            if (this->beta.size() == 0) {
                if (this->alpha != 1) {
                    assert(false);
                } else {
#ifdef DBGDEBUG
                    assert(this->child[0] == NULL);
                    assert(this->child[1] == NULL);
                    std::cout << "--------\n";
#endif
                    return;
                }
            }
            this->move_label_down(lsb(this->alpha));
            bv_t bv(this->beta.size()+addother);
            size_t i=(leftorright ? 0 : addother);
            copy_bits(bv, this->beta, i);
            this->set_beta(beta_t(bv));
            //this->beta = beta_t(bv);
            //sdsl::util::init_support(this->rank1, &(this->beta));
            //sdsl::util::init_support(this->rank0, &(this->beta));
#ifdef DBGDEBUG
            this->print(true);
            std::cout << "--------\n";
#endif
            if (this->child[0] == NULL) {
#ifdef DBGDEBUG
                std::cout << "Adding new leaf, since destroyed\n";
#endif
                this->child[0] = new Node();
                this->child[0]->alpha=1;
            }
            this->child[0]->append(other, leftorright, 0, addother);
            return;
        }
#ifdef DBGDEBUG
        other->print(true);
        std::cout << "\n";
#endif

        if (this->beta.size() == 0) {
#ifdef DBGDEBUG
            assert(this->child[0] == NULL);
            assert(this->child[1] == NULL);
#endif
            if (this->alpha != 1) {
                this->breakleaf(addme);
#ifdef DBGDEBUG
                assert(this->beta.size() > 0);
                assert(this->child[0] != NULL);
                assert(this->child[0]->alpha == 1);
#endif
            }
        }
        if (other->beta.size() == 0) {
#ifdef DBGDEBUG
            assert(other->child[0] == NULL);
            assert(other->child[1] == NULL);
#endif
            if (other->alpha != 1) {
                other->breakleaf(addother);
#ifdef DBGDEBUG
                assert(other->beta.size() > 0);
                assert(other->child[0] != NULL);
                assert(other->child[0]->alpha == 1);
#endif
            }
        }
        if (other->beta.size() == 0) {
            if (other->alpha != 1) {
                std::cerr << "Bad leaf\n";
                std::cerr << other->alpha << "\n";
                assert(false);
            }
            if (this->beta.size() == 0) {
#ifdef DBGDEBUG
                std::cout << "Done with this branch";
                assert(this->alpha == 1 && other->alpha == 1);
#endif
                return;
            } else {
#ifdef DBGDEBUG
                std::cout << "Fixing left children, append to right\n";
#endif
                this->append(NULL, 1, 0, addother);
                return;
            }
        }
        if (this->beta.size() == 0) {
#ifdef DBGDEBUG
            assert(this->alpha == 1);
            assert(this->child[0] == NULL);
            assert(this->child[1] == NULL);
#endif
            this->alpha = other->alpha;
            this->beta = other->beta;
            this->rank1 = other->rank1;
            this->rank0 = other->rank0;
            if (other->child[0] != NULL)
                this->child[0] = new Node(other->child[0], false);
            if (other->child[1] != NULL)
                this->child[1] = new Node(other->child[1], false);
#ifdef DBGDEBUG
            std::cout << "Fixing left children, append to left\n";
#endif
            this->append(NULL, 0, 0, addme);
            return;
        }
#ifdef DBGDEBUG
        std::cout << "Fixing internal node\n";
#endif
        
        intvec_t alphas;
        alphas.push_back(this->alpha);
        alphas.push_back(other->alpha);
        bit_unset(alphas[0], msb(alphas[0]));
        bit_unset(alphas[1], msb(alphas[1]));
        annot_t overlap = (alphas[0] ^ alphas[1]);
        size_t tlen=msb(this->alpha);
        size_t olen=msb(other->alpha);
        size_t ol=0;
        if (overlap == 0) {
            ol = std::min(tlen, olen);
        } else {
            ol = lsb(overlap);
            if (ol > tlen || ol > olen) {
#ifdef DBGDEBUG
                assert((ol > tlen) ^ (ol > olen));
#endif
                ol = std::min(tlen, olen);
            }
        }
        if (olen != tlen || overlap != 0) {
#ifdef DBGDEBUG
            std::cout << "Different prefix, reshaping\n";
            assert(this->beta.size() > 0 || this->alpha == 1);
            assert(other->beta.size() > 0 || other->alpha == 1);
#endif
            this->move_label_down(ol, addme);
            other->move_label_down(ol, addother);
#ifdef DBGDEBUG
            assert(this->alpha == other->alpha);
#endif
        }
#ifdef DBGDEBUG
        std::cout << "Merging\n";
#endif

        //fix alphas
        bv_t bv(this->beta.size()+other->beta.size());
        copy_bits(bv, this->beta);
        copy_bits(bv, other->beta, this->beta.size());
#ifdef DBGDEBUG
        std::cout << bv << "\n";
        this->print(true);
        std::cout << "-\n";
        other->print(true);
        std::cout << "--------\n";
#endif
        this->rank0.set_vector(&(this->beta));
        other->rank0.set_vector(&(other->beta));
        this->rank1.set_vector(&(this->beta));
        other->rank1.set_vector(&(other->beta));
        if (this->child[0] != NULL) {
            #pragma omp task if(this->rank0(this->beta.size()) > TASKMIN || other->rank0(other->beta.size()) > TASKMIN)
            this->child[0]->append(other->child[0], 0, this->rank0(this->beta.size()), other->rank0(other->beta.size()));
        } else if (other->child[0] != NULL) {
            if (this->rank0(this->beta.size()) > 0) {
                std::cerr << this->beta << "\n";
                assert(false);
            }
            this->child[0] = new Node(other->child[0], false);
        }
        if (this->child[1] != NULL) {
            #pragma omp task if(this->rank1(this->beta.size()) > TASKMIN || other->rank1(other->beta.size()) > TASKMIN)
            this->child[1]->append(other->child[1], 1, this->rank1(this->beta.size()), other->rank1(other->beta.size()));
        } else if (other->child[1] != NULL) {
            if (this->rank1(this->beta.size()) > 0) {
                std::cerr << this->beta << "\n";
                assert(false);
            }
            this->child[1] = new Node(other->child[1], false);
        }
        #pragma omp taskwait
        this->set_beta(beta_t(bv));
        //this->beta = beta_t(bv);
        //sdsl::util::init_support(this->rank1, &(this->beta));
        //sdsl::util::init_support(this->rank0, &(this->beta));
    }

    //TODO: rewrite without recursion
    Node* Node::merge(std::vector<Node*>::iterator begin, std::vector<Node*>::iterator end) {
        if (end == begin)
            return NULL;
        if (end-begin == 1) {
            return *begin;
        }
        if (end-begin == 2) {
            (*begin)->append(*(begin+1), false);
            delete (*(begin+1));
            return *begin;
        }
        Node *a, *b;
        #pragma omp task shared(a)
        a = merge(begin, begin+((end-begin)/2));
        #pragma omp task shared(b)
        b = merge(begin+((end-begin)/2), end);
        #pragma omp taskwait
        a->append(b, false);
        delete b;
        return a;
    }

    WTR WTR::merge(std::vector<WTR*>::iterator begin, std::vector<WTR*>::iterator end) {
        std::vector<Node*> nodes(end-begin);
        auto jt = nodes.begin();
        size_t totalsize=0;
        for (auto it = begin; it != end; ++it) {
            assert(*it != NULL);
            *jt = (*it)->root;
            ++jt;
            totalsize += (*it)->size;
            (*it)->root = NULL;
        }
        WTR wtr;
        if (end > begin) {
            Node *a;
            #pragma omp parallel
            #pragma omp single nowait
            a = Node::merge(nodes.begin(), nodes.end());
            wtr.size = totalsize;
            wtr.root = a;
        }
        return wtr;
    }


    WTR::WTR(intvec_t *iv, size_t osize, size_t batch) {
        this->root = new Node();
        if (batch != 0 && iv->size() != 0) {
            size_t tsize=0;
            prefix_t prefix;
            //std::cout << "Computing alpha\n";
            prefix = longest_prefix(iv->begin(), std::min(iv->begin()+batch, iv->end()), 0, osize); 
            //std::cout << "Computing wavelet trie\n";
            tsize = iv->size();
            #pragma omp parallel
            #pragma omp single nowait
            this->root->add(iv, 0, osize, prefix); 
#ifdef DBGDEBUG
            this->print();
            std::cout << "-------------\n\n";
#endif
            this->size = tsize;
            //Node *other;
            std::vector<Node*> others(tsize/batch,NULL);
            #pragma omp parallel for shared(others)
            for (size_t cursize = batch; cursize < tsize; cursize += batch) {
                //other = new Node();
                others[cursize/batch-1] = new Node();
                //std::cout << "Computing alpha\n";
                //#pragma omp parallel
                //#pragma omp single nowait
                //std::cout << "Computing wavelet trie\n";
                prefix = longest_prefix(iv->begin()+cursize, std::min(iv->begin()+cursize+batch, iv->end()), 0, osize); 
                #pragma omp parallel
                #pragma omp single nowait
                others[cursize/batch-1]->add(iv, 0, osize, prefix); 
                if ((cursize/batch-1) % 100 == 0)
                    std::cout << "\n";
                std::cout << "." << std::flush;
                
            }
            if (others.size() && others[0]) {
                std::cout << "\nMerging\n";
                Node *a;
                #pragma omp parallel
                #pragma omp single nowait
                a = Node::merge(others.begin(), others.end());
                #pragma omp parallel
                #pragma omp single nowait
                this->root->append(a, false);
                delete a;
            }
        }
    }

    annot_t WTR::at(size_t i) {
        annot_t annot = 0;
        mpz_t& annot_a = annot.backend().data();
        Node *curnode = this->root;
        size_t len=0;
        uint8_t curbit=0;
        mpz_t temp;
        if (curnode != NULL) {
            while (curnode->beta.size()) {
                mpz_t& cur = curnode->alpha.backend().data();
                mpz_init(temp);
                mpz_mul_2exp(temp, cur, len);
                mpz_ior(annot_a, annot_a, temp);
                mpz_clear(temp);
                //annot |= curnode->alpha << len;
                //annot += curnode->alpha << len;
                //len += msb(curnode->alpha);
                len += mpz_sizeinbase(cur, 2);
                curbit = curnode->beta[i];
                if (curbit) {
                    curnode->rank1.set_vector(&(curnode->beta));
                    i = curnode->rank1(i);
                } else {
                    bit_unset(annot, len - 1);
                    curnode->rank0.set_vector(&(curnode->beta));
                    i = curnode->rank0(i);
                }
                curnode = curnode->child[curbit];
                if (curnode == NULL) {
                    std::cerr << "Missing node\n";
                    assert(false);
                }
            }
            assert(curnode != NULL);
            //annot |= curnode->alpha << len;
            mpz_t& cur = curnode->alpha.backend().data();
            mpz_init(temp);
            mpz_mul_2exp(temp, cur, len);
            mpz_ior(annot_a, annot_a, temp);
            mpz_clear(temp);
            bit_unset(annot, msb(annot));
        }
        return annot;
    }

    bool WTR::reconstruct(intvec_t::iterator ivb, intvec_t::iterator ive) {
        annot_t annot=0;
        for (auto it=ivb; it!= ive; ++it) {
            annot = this->at(it-ivb);
            if (annot != *it) {
                std::cerr << "Fail " << it-ivb << " " << *it << " " << annot << "\n";
                return false;
            }
        }
        return true;
    }

    bool WTR::reconstruct(intvec_t &iv) {
        return this->reconstruct(iv.begin(), iv.end());
    }

    std::pair<intvec_t*, intvec_t*> construct(std::ifstream &pfile, size_t begin=0, size_t end=MAXNUM) {
        intvec_t* iv = new intvec_t();
        std::string line, ed;
        annot_t cannot, ctemp;
        size_t edi = 0, maxedi=0;
        while (pfile >> line) {
            std::istringstream sfile(line);
            cannot = 0;
            while (std::getline(sfile, ed, ',')) {
                edi = atol(ed.c_str());
#ifdef DBGDEBUG
                std::cout << edi << ",";
#endif
                if (edi) {
                    maxedi = std::max(maxedi, edi);
                    ctemp = 0;
                    bit_set(ctemp, edi-1);
                    if (ctemp == 0) {
                        std::cerr << "Fail at " << edi << "\n";
                        assert(false);
                    }
                    cannot |= ctemp;
                    //ctemp = cannot | (annot_t(1) << (edi-1));
                    //if (ctemp == cannot) {
                    //    std::cerr << "Fail at " << edi << "\n";
                    //    assert(false);
                    //}
                    //cannot = ctemp;
                }
            }
            //if (edi >= begin && edi < end)
            iv->push_back(cannot);
            if (iv->size() == begin)
                break;
#ifdef DBGDEBUG
            std::cout << "\t" << iv->back() << "\n";
#endif
        }
#ifdef DBGDEBUG
        std::cout << "\n";
#endif
        return std::make_pair(iv, new intvec_t(maxedi,0));
    }

    size_t popcount(const cpp_int &a) {
        /*
        size_t as = a.backend().size();
        size_t apc=0;
        auto al = a.backend().limbs();
        size_t i=0;
        for (;i<as;++i) {
            apc += __builtin_popcount(*al);
            ++al;
        }
        return apc;
        */
        return mpz_popcount(a.backend().data());
    }

    /*
    size_t popcount(const array_int::array_int &a) {
        return a.indices.size();
    }
    */

    void Node::print(std::ostream &os, bool recursive) {
        os << this->alpha << ";" << this->beta << "\t";
        if (this->child[0] != NULL)
            os << "l";
        if (this->child[1] != NULL)
            os << "r";
        os << "\n";
        if (this->child[0] != NULL)
            this->child[0]->print(os);
        if (this->child[1] != NULL)
            this->child[1]->print(os);
    }

    void Node::print(bool recursive) {
        this->print(std::cout, recursive);
    }

    void WTR::print(std::ostream &os) {
        os << "Printing\n";
        this->root->print(os, true);
        os << "\n";
    }

    void WTR::print() {
        this->print(std::cout);
    }

    WTR::WTR() {
    }

}

#endif
