#pragma once

#include <assert.h>
#include <stdio.h>
#include "port/port.h"
#include "util/arena.h"
#include "util/random.h"

namespace leveldb{
    class Arena;

    template <typename Key, class Comparator>
        class SkipList{
            private:
                struct Node;

            public:
                explicit SkipList(Comparator com,Arena* arena);

                void Insert(const Key& key);

                bool Comtain(const Key& key) const;

                class Iterator{
                    public:
                        explicit Iterator(const SkipList* list);

                        bool Valid() const;
                        const Key& key() const;
                        void Next();
                        void Prev();

                        void Seek(const Key& target);
                        void SeekToFirst();
                        void SeekToLast();

                    private:
                        const SkipList* list_;
                        Node* node_;
                };

            private:
                enum {kMaxHeight = 12};

                Comparator const compare_;
                Arena* const arena_;
                Node* const head_;

                port::AtomicPointer max_height_;

                inline int GetMaxHeight()const{
                    return static_cast<uintptr_t>(max_height_.NoBarrie_Load());
                }

                Random rnd_;
                Node* NewNode(const Key& key,int height);
                int RandomHeight();
                bool Equal(const Key& a,const Key& b)const {return compare_(a,b)==0;}
                bool KeyIsAfterNode(const Key& key,Node* n)const;
                Node* FindGreaterOrEqual(const Key& key,Node** prev) const;
                Node* FindLessThan(const Key& key)const;
                Node* FindLast()const;

                //阻止拷贝构造和赋值构造
                SkipList(const SkipList&);
                void operator=(const SkipList&);

        };


    template<typename Key,class Comparator>
        struct SkipList<Key,Comparator>::Node{
            explicit Node(const Key& key):key_(key){}
            Key const key;

            Node* Next(int n){
                assert(n>=0);
                return reinterpret_cast<Node*>(next_[n].Acquire_Load());
            }

            void SetNext(int n,Node* x){
                assert(n>=0);
                next_[n].Release_Store(x);
            }

            Node* NoBarrier_Next(int n){
                assert(n>=0);
                return reinterpret_cast<Node*>(next_[n].NoBarrie_Load());
            }

            Node* NoBarrier_SetNext(int n,Node* x){
                assert(n>=0);
                next_[n].NoBarrier_Store(x);
            }

            private:
                port::AtomicPointer next_[1];
        };

    template<typename Key,class Comparator>
        typename SkipList<Key,Comparator>::Node* SkipList<Key,Comparator>::NewNode(const Key& key,int height){
            char* mem = arena->AllocateAligned(sizeof(Node)+sizeof(port::AtomicPointer)*(height-1));//height-1的原因Node里面已经有个next_[1]了
            return new(mem)Node(key);
        }

    template<typename Key,class Comparator>
        inline SkipList<Key,Comparator>::Iterator::Iterator(const SkipList* list){
            list_ = list;
            node_ = nullptr;
        }

    template<typename Key,Class Comparator>
        inline bool SkipList<Key,Comparator>::Iterator::Valid() const{
            return node_ != nullptr;
        }

    template<typename Key,Class Comparator>
        inline const Key& SkipList<Key,Comparator>::Iterator::Key() const{
            assert(Valid());
            return node_->key;
        }

    template<typename Key,Class Comparator>
        inline void SkipList<Key,Comparator>::Iterator::Nex(){
            assert(Valid());
            node_ = node_->Next[0];
        }

    template<typename Key,Class Comparator>
        inline void SkipList<Key,Comparator>::Iterator::Prev(){
            assert(Valid());
            node_ = list_->FindLessThan(node_->key);
            if(node_ == list_->head_){
                node_ = nullptr;
            }
        }

    template<typename Key,Class Comparator>
        inline void SkipList<Key,Comparator>::Iterator::Seek(const Key& target){
            node_ = list_->FindGreaterOrEqual(target,nullptr);
        }

    template<typename Key,Class Comparator>
        inline void SkipList<Key,Comparator>::Iterator::SeekToFirst(){
            node_ = list_->head_->Next[0];
        }

    template<typename Key,Class Comparator>
        inline void SkipList<Key,Comparator>::Iterator::SeekToLast(){
            node_ = list_->FindLast();
            if(list_ == list_->head_){
                node_ = nullptr;
            }
        }

    template<typename Key,Class Comparator>
        int SkipList<Key,Comparator>::RandomHeight(){//四分之一的概率增加高度
            static const unsigned int kBranching = 4;
            int height = 1;
            while(height<kMaxHeight && (rnd_.Next()%kBranching == 0)){
                height++;
            }
            return height;
        }

    template<typename Key,Class Comparator>
        bool SkipList<Key,Comparator>::KeyIsAfterNode(const Key& key,Node* n){
            return (n!=nullptr) && compare_(n->key,key)<0;
        }

    template<typename Key,Class Comparator>
        typename SkipList<Key,Comparator>::Node* SkipList<Key,Comparator>::FindGreaterOrEqual(const Key& key,Node** prev)const{
            Node* x = head_;
            int level = GetMaxHeight()-1;
            while(true){
                Node* next = x->Next(level);
                if(KeyIsAfterNode(key,next)){
                    x = next;
                }else{
                    if(prev != nullptr){prev[level] = x;}
                    if(level == 0){return next;}
                    else{
                        level--;
                    }
                }
            }
        }

    template<typename Key,Class Comparator>
        typename SkipList<Key,Comparator>::Node* SkipList<Key,Comparator>::FindLessThan(const Key& key)const{
            Node* x = head_;
            int level = GetMaxHeight()-1;
            while(true){
                assert(x==head_ || compare_(x->key,key)<0);
                Node* next = x->Next(level);
                if(next==nullptr || compare_(next->key,key)>=0){
                    if(level == 0){
                        reuturn x;
                    }else{
                        level--;
                    }
                }
                else{
                    x = next;
                }
            }
        }

    template<typename Key,Class Comparator>
        typename SkipList<Key,Comparator>::Node* SkipList<Key,Comparator>::FindLast()const{
            Node* x = head_;
            int level = GetMaxHeight()-1;
            while(true){
                Node* next = x->Next(level);
                if(next==nullptr){
                    if(level==0){
                        return x;
                    }else{
                        level--;
                    }

                }else{
                    x=next;
                }
            }
        }

    template<typename Key,Class Comparator>
        SkipList<Key,Comparator>::SkipList(Comparator cmp,Arena* arena):compare_(cmp),arena_(arena),head_(NewNode(0,kMaxHeight)),
            max_height_(reinterpret_cast<void*>(1)),rnd(0xdeadbeef){
                for(int i=0;i<kMaxHeight;i++){
                    head_->SetNext(i,nullptr);
                }
        }

    template<typename Key,Class Comparator>
        void SkipList<Key,Comparator>::Insert(const Key& Key){
            Node* prev[kMaxHeight];
            Node* x = FindGreaterOrEqual(key,prev);

            assert(x==nullptr || !Equal(key,x->key));
            int height = RandomHeight();
            if(height > GetMaxHeight()){
                for(int i=GetMaxHeight();i<height;i++){
                    prev[i] = head_;
                }
                max_height_.NoBarrier_Store(reinterpret_cast<void*>(height));
            }

            x = NewNode(key,height);
            for(int i=0;i<height;i++){
                x->NoBarrier_SetNext(i,prev[i]->NoBarrier_Next(i));
                prev[i]->SetNext(i,x);
            }
        }

    template<typename Key,Class Comparator>
        bool SkipList<Key,Comparator>::Comtains(const Key& key)const{
            Node* x = FindGreaterOrEqual(key,nullptr);
            if(x != nullptr && Equal(key,x->key)){
                return true;
            }else{
                return false;
            }

        }
