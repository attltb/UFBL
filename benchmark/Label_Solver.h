#ifndef HEADER_LABELSOLVER
#define HEADER_LABELSOLVER

// Union-Find (UF) with path compression (PC) as in:
// Two Strategies to Speed up Connected Component Labeling Algorithms
// Kesheng Wu, Ekow Otoo, Kenji Suzuki
class UFPC {
    // Maximum number of labels (included background) = 2^(sizeof(unsigned) x 8)
public:
    void Alloc(unsigned max_length) {
        P_ = new unsigned[max_length];
    }
    void Dealloc() {
        delete[] P_;
    }
    void Setup() {
        P_[0] = 0;	 // First label is for background pixels
        length_ = 1;
    }
    unsigned NewLabel() {
        P_[length_] = length_;
        return length_++;
    }
    unsigned GetLabel(unsigned index) {
        return P_[index];
    }

    unsigned Merge(unsigned i, unsigned j)
    {
        // FindRoot(i)
        unsigned root(i);
        while (P_[root] < root) {
            root = P_[root];
        }
        if (i != j) {
            // FindRoot(j)
            unsigned root_j(j);
            while (P_[root_j] < root_j) {
                root_j = P_[root_j];
            }
            if (root > root_j) {
                root = root_j;
            }
            // SetRoot(j, root);
            while (P_[j] < j) {
                unsigned t = P_[j];
                P_[j] = root;
                j = t;
            }
            P_[j] = root;
        }
        // SetRoot(i, root);
        while (P_[i] < i) {
            unsigned t = P_[i];
            P_[i] = root;
            i = t;
        }
        P_[i] = root;
        return root;
    }
    unsigned Flatten()
    {
        unsigned k = 1;
        for (unsigned i = 1; i < length_; ++i) {
            if (P_[i] < i) {
                P_[i] = P_[P_[i]];
            }
            else {
                P_[i] = k;
                k = k + 1;
            }
        }
        return k;
    }

private:
    unsigned* P_;
    unsigned length_;
};

#endif