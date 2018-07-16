//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 24-06-18.
//

#include <vector>

#include "grammar/re_pair.h"

#include "repair/basics.h"
#include "repair/array.h"
#include "repair/records.h"
#include "repair/heap.h"
#include "repair/hash.h"


struct grammar::RePairBasicEncoder::InternalData {
  int u;  // |text| and later current |C| with gaps
  int c;  // real |C|
  int alph; // max used terminal symbol
  int n;  // |R|
  Trarray Rec;  // records
  Theap Heap; // special heap of pairs
  Thash Hash; // hash table of pairs
  Tlist *L; // |L| = c;

  const float factor = 0.75;
  const int minsize = 256;  // to avoid many reallocs at small sizes, should be ok as is
};


void grammar::RePairBasicEncoder::prepare(int C[], std::size_t len) {
  data_.reset(new InternalData);

  int i, id;
  Tpair pair;
  data_->c = data_->u = len;
  data_->alph = 0;
  for (i = 0; i < data_->u; i++) {
    if (C[i] > data_->alph) data_->alph = C[i];
  }
  data_->n = ++data_->alph;
  data_->Rec = createRecords(data_->factor, data_->minsize);
  data_->Heap = createHeap(data_->u, &data_->Rec, data_->factor, data_->minsize);
  data_->Hash = createHash(256 * 256, &data_->Rec);
  data_->L = (Tlist *) malloc(data_->u * sizeof(Tlist));
  assocRecords(&data_->Rec, &data_->Hash, &data_->Heap, data_->L);
  for (i = 0; i < data_->c - 1; i++) {
    pair.left = C[i];
    pair.right = C[i + 1];
    id = searchHash(data_->Hash, pair);
    if (id == -1) // new pair, insert
    {
      id = insertRecord(&data_->Rec, pair);
      data_->L[i].next = -1;
    } else {
      data_->L[i].next = data_->Rec.records[id].cpos;
      data_->L[data_->L[i].next].prev = i;
      incFreq(&data_->Heap, id);
    }
    data_->L[i].prev = -id - 1;
    data_->Rec.records[id].cpos = i;
//    if (PRNL && (i%10000 == 0)) printf ("Processed %i chars\n",i);
  }
  purgeHeap(&data_->Heap);
}


int grammar::RePairBasicEncoder::repair(int C[], std::size_t, std::function<void(int, int, int)> _writer) {
  int oid, id, cpos;
  Trecord *rec, *orec;
  Tpair pair;
//  if (fwrite(&alph,sizeof(int),1,R) != 1) return -1;
//  if (PRNC) prnC();

//  std::vector<int> lengths;
  while (data_->n + 1 > 0) {
//    if (PRNR) prnRec();
    oid = extractMax(&data_->Heap);
    if (oid == -1) break; // the end!!
    orec = &data_->Rec.records[oid];
    cpos = orec->cpos;


    // Adding a new rule to the output
    int lrule = get_rule_span_length(orec->pair.left) + get_rule_span_length(orec->pair.right);
//    if (orec->pair.left < data_->alph) lrule++;
//    else lrule += lengths[orec->pair.left - data_->alph];
//
//    if (orec->pair.right < data_->alph) lrule++;
//    else lrule += lengths[orec->pair.right - data_->alph];

    _writer(orec->pair.left, orec->pair.right, lrule);
    rules_span_length_.push_back(lrule);
    rules_height_.push_back(std::max(get_rule_height(orec->pair.left), get_rule_height(orec->pair.right)) + 1);


//    if (fwrite (&orec->pair,sizeof(Tpair),1,R) != 1) return -1;
//    if (PRNP)
//    { printf("Chosen pair %i = (",n);
//      prnSym(orec->pair.left);
//      printf(",");
//      prnSym(orec->pair.right);
//      printf(") (%i occs)\n",orec->freq);
//    }
    while (cpos != -1) {
      int ant, sgte, ssgte;
      // replacing bc->e in abcd, b = cpos, c = sgte, d = ssgte
      if (C[cpos + 1] < 0) sgte = -C[cpos + 1] - 1;
      else sgte = cpos + 1;
      if ((sgte + 1 < data_->u) && (C[sgte + 1] < 0)) ssgte = -C[sgte + 1] - 1;
      else ssgte = sgte + 1;
      // remove bc from L
      if (data_->L[cpos].next != -1) data_->L[data_->L[cpos].next].prev = -oid - 1;
      orec->cpos = data_->L[cpos].next;
      if (ssgte != data_->u) // there is ssgte
      {    // remove occ of cd
        pair.left = C[sgte];
        pair.right = C[ssgte];
        id = searchHash(data_->Hash, pair);
        if (id != -1) // may not exist if purgeHeap'd
        {
          if (id != oid) decFreq(&data_->Heap, id); // not to my pair!
          if (data_->L[sgte].prev != NullFreq) //still exists(not removed)
          {
            rec = &data_->Rec.records[id];
            if (data_->L[sgte].prev < 0) // this cd is head of its list
              rec->cpos = data_->L[sgte].next;
            else data_->L[data_->L[sgte].prev].next = data_->L[sgte].next;
            if (data_->L[sgte].next != -1) // not tail of its list
              data_->L[data_->L[sgte].next].prev = data_->L[sgte].prev;
          }
        }
        // create occ of ed
        pair.left = data_->n;
        id = searchHash(data_->Hash, pair);
        if (id == -1) // new pair, insert
        {
          id = insertRecord(&data_->Rec, pair);
          rec = &data_->Rec.records[id];
          data_->L[cpos].next = -1;
        } else {
          incFreq(&data_->Heap, id);
          rec = &data_->Rec.records[id];
          data_->L[cpos].next = rec->cpos;
          data_->L[data_->L[cpos].next].prev = cpos;
        }
        data_->L[cpos].prev = -id - 1;
        rec->cpos = cpos;
      }
      if (cpos != 0) // there is ant
      {    // remove occ of ab
        if (C[cpos - 1] < 0) {
          ant = -C[cpos - 1] - 1;
          if (ant == cpos) // sgte and ant clashed -> 1 hole
            ant = cpos - 2;
        } else ant = cpos - 1;
        pair.left = C[ant];
        pair.right = C[cpos];
        id = searchHash(data_->Hash, pair);
        if (id != -1) // may not exist if purgeHeap'd
        {
          if (id != oid) decFreq(&data_->Heap, id); // not to my pair!
          if (data_->L[ant].prev != NullFreq) //still exists (not removed)
          {
            rec = &data_->Rec.records[id];
            if (data_->L[ant].prev < 0) // this ab is head of its list
              rec->cpos = data_->L[ant].next;
            else data_->L[data_->L[ant].prev].next = data_->L[ant].next;
            if (data_->L[ant].next != -1) // it is not tail of its list
              data_->L[data_->L[ant].next].prev = data_->L[ant].prev;
          }
        }
        // create occ of ae
        pair.right = data_->n;
        id = searchHash(data_->Hash, pair);
        if (id == -1) // new pair, insert
        {
          id = insertRecord(&data_->Rec, pair);
          rec = &data_->Rec.records[id];
          data_->L[ant].next = -1;
        } else {
          incFreq(&data_->Heap, id);
          rec = &data_->Rec.records[id];
          data_->L[ant].next = rec->cpos;
          data_->L[data_->L[ant].next].prev = ant;
        }
        data_->L[ant].prev = -id - 1;
        rec->cpos = ant;
      }
      C[cpos] = data_->n;
      if (ssgte != data_->u) C[ssgte - 1] = -cpos - 1;
      C[cpos + 1] = -ssgte - 1;
      data_->c--;
      orec = &data_->Rec.records[oid]; // just in case of Rec.records realloc'd
      cpos = orec->cpos;
    }
//    if (PRNC) prnC();
    removeRecord(&data_->Rec, oid);
    data_->n++;
    purgeHeap(&data_->Heap); // remove freq 1 from heap
    if (data_->c < data_->factor * data_->u) // compact C
      //todo compact one time at the end
    {
      int i, ni;
      i = 0;
      for (ni = 0; ni < data_->c - 1; ni++) {
        C[ni] = C[i];
        data_->L[ni] = data_->L[i];
        if (data_->L[ni].prev < 0) {
          if (data_->L[ni].prev != NullFreq) // real ptr
            data_->Rec.records[-data_->L[ni].prev - 1].cpos = ni;
        } else data_->L[data_->L[ni].prev].next = ni;
        if (data_->L[ni].next != -1) data_->L[data_->L[ni].next].prev = ni;
        i++;
        if (C[i] < 0) i = -C[i] - 1;
      }
      C[ni] = C[i];
      data_->u = data_->c;
      C = (int *) realloc (C, data_->c * sizeof(int));
      data_->L = (Tlist *) realloc (data_->L, data_->c * sizeof(Tlist));
      assocRecords(&data_->Rec, &data_->Hash, &data_->Heap, data_->L);
    }
  }

  for (int i = 0, j = 0; i < data_->c; ++i) {
    C[i] = C[j];
    ++j;
    if (C[j] < 0) j = -C[j] - 1;
  }
  data_->u = data_->c;
  C = (int *) realloc (C, data_->c * sizeof(int));

  return data_->u;
}


void grammar::RePairBasicEncoder::destroy() {
  free(data_->L);
  destroyHeap(&data_->Heap);
  destroyHash(&data_->Hash);

  rules_span_length_.clear();
  rules_height_.clear();
}


int grammar::RePairBasicEncoder::get_rule_span_length(int _rule) const {
  return (_rule < data_->alph) ? 1 : rules_span_length_[_rule - data_->alph];
}


int grammar::RePairBasicEncoder::get_rule_height(int _rule) const {
  return (_rule < data_->alph) ? 0 : rules_height_[_rule - data_->alph];
}


int grammar::RePairBasicEncoder::sigma() const {
  return data_->alph - 1;
}

