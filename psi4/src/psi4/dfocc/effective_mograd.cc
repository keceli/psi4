/*
 * @BEGIN LICENSE
 *
 * Psi4: an open-source quantum chemistry software package
 *
 * Copyright (c) 2007-2022 The Psi4 Developers.
 *
 * The copyrights for code used from other parties are included in
 * the corresponding files.
 *
 * This file is part of Psi4.
 *
 * Psi4 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Psi4 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with Psi4; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * @END LICENSE
 */

#include <cmath>
#include "psi4/libqt/qt.h"
#include "defines.h"
#include "dfocc.h"

using namespace psi;

namespace psi {
namespace dfoccwave {

void DFOCC::effective_mograd() {
    // outfile->Printf("\n effective_mograd is starting... \n");
    outfile->Printf("\tForming effective orbital gradient...\n");

    if (reference_ == "RESTRICTED")
        WvoA->form_vo(WorbA);
    else if (reference_ == "UNRESTRICTED") {
        WvoA->form_vo(WorbA);
        WvoB->form_vo(WorbB);
    }

    if (nfrzc > 0) {
        z_vector_fc();
        fc_grad_terms();
    }

    // outfile->Printf("\n effective_mograd done. \n");
}  // end effective_mograd

//=======================================================
//      Strictly canonical MOGRAD
//=======================================================
void DFOCC::effective_mograd_sc() {
    // outfile->Printf("\n effective_mograd is starting... \n");
    outfile->Printf("\tForming strictly canonical effective orbital gradient...\n");

    if (reference_ == "RESTRICTED")
        WvoA->form_vo(WorbA);
    else if (reference_ == "UNRESTRICTED") {
        WvoA->form_vo(WorbA);
        WvoB->form_vo(WorbB);
    }

    // OO and VV gradient terms
    z_vector_oo();
    z_vector_vv();
    oo_grad_terms();
    vv_grad_terms();
}  // end effective_mograd_sc

//=======================================================
//      Z-Vector: OO Block
//=======================================================
void DFOCC::z_vector_oo() {
    if (reference_ == "RESTRICTED") {
        // Build Zoo
        ZklA = SharedTensor2d(new Tensor2d("Zvector (K|L)", noccA, noccA));
#pragma omp parallel for
        for (int k = 0; k < noccA; k++) {
            for (int l = 0; l < noccA; l++) {
                if (k != l) {
                    double value = FockA->get(k, k) - FockA->get(l, l);
                    if (std::fabs(value) > tol_pcg) ZklA->set(k, l, -WorbA->get(k, l) / (2.0 * value));
                }
            }
        }
        // ZklA->print();
    }  // end if (reference_ == "RESTRICTED")

    else if (reference_ == "UNRESTRICTED") {
        // Build Zoo
        // Alpha
        ZklA = SharedTensor2d(new Tensor2d("Zvector (K|L)", noccA, noccA));
#pragma omp parallel for
        for (int k = 0; k < noccA; k++) {
            for (int l = 0; l < noccA; l++) {
                if (k != l) {
                    double value = FockA->get(k, k) - FockA->get(l, l);
                    if (std::fabs(value) > tol_pcg) ZklA->set(k, l, -WorbA->get(k, l) / (2.0 * value));
                }
            }
        }

        // Beta
        ZklB = SharedTensor2d(new Tensor2d("Zvector (k|l)", noccB, noccB));
#pragma omp parallel for
        for (int k = 0; k < noccB; k++) {
            for (int l = 0; l < noccB; l++) {
                if (k != l) {
                    double value = FockB->get(k, k) - FockB->get(l, l);
                    if (std::fabs(value) > tol_pcg) ZklB->set(k, l, -WorbB->get(k, l) / (2.0 * value));
                }
            }
        }

    }  // end if (reference_ == "UNRESTRICTED")

}  // end z_vector_oo

//=======================================================
//      Z-Vector: VV Block
//=======================================================
void DFOCC::z_vector_vv() {
    if (reference_ == "RESTRICTED") {
        // Build Zvv
        ZcdA = SharedTensor2d(new Tensor2d("Zvector (C|D)", nvirA, nvirA));
#pragma omp parallel for
        for (int c = 0; c < nvirA; c++) {
            for (int d = 0; d < nvirA; d++) {
                if (c != d) {
                    double value = FockA->get(c + noccA, c + noccA) - FockA->get(d + noccA, d + noccA);
                    if (std::fabs(value) > tol_pcg) ZcdA->set(c, d, -WorbA->get(c + noccA, d + noccA) / (2.0 * value));
                }
            }
        }
        // ZcdA->print();
    }  // end if (reference_ == "RESTRICTED")

    else if (reference_ == "UNRESTRICTED") {
        // Build Zvv
        // Alpha
        ZcdA = SharedTensor2d(new Tensor2d("Zvector (C|D)", nvirA, nvirA));
#pragma omp parallel for
        for (int c = 0; c < nvirA; c++) {
            for (int d = 0; d < nvirA; d++) {
                if (c != d) {
                    double value = FockA->get(c + noccA, c + noccA) - FockA->get(d + noccA, d + noccA);
                    if (std::fabs(value) > tol_pcg) ZcdA->set(c, d, -WorbA->get(c + noccA, d + noccA) / (2.0 * value));
                }
            }
        }

        // Beta
        ZcdB = SharedTensor2d(new Tensor2d("Zvector (c|d)", nvirB, nvirB));
#pragma omp parallel for
        for (int c = 0; c < nvirB; c++) {
            for (int d = 0; d < nvirB; d++) {
                if (c != d) {
                    double value = FockB->get(c + noccB, c + noccB) - FockB->get(d + noccB, d + noccB);
                    if (std::fabs(value) > tol_pcg) ZcdB->set(c, d, -WorbB->get(c + noccB, d + noccB) / (2.0 * value));
                }
            }
        }

    }  // end if (reference_ == "UNRESTRICTED")

}  // end z_vector_vv

//=======================================================
//      Z-Vector: ACO-FC Block
//=======================================================
void DFOCC::z_vector_fc() {
    if (reference_ == "RESTRICTED") {
        // Build Zoo
        ZklA = SharedTensor2d(new Tensor2d("Zvector <I|FC>", naoccA, nfrzc));
#pragma omp parallel for
        for (int k = 0; k < naoccA; k++) {
            for (int l = 0; l < nfrzc; l++) {
                double value = FockA->get(k + nfrzc, k + nfrzc) - FockA->get(l, l);
                ZklA->set(k, l, -WorbA->get(k + nfrzc, l) / (2.0 * value));
            }
        }
        ZlkA = SharedTensor2d(new Tensor2d("Zvector <FC|I>", nfrzc, naoccA));
        ZlkA = ZklA->transpose();

    }  // end if (reference_ == "RESTRICTED")

    else if (reference_ == "UNRESTRICTED") {
        // Build Zoo
        // Alpha
        ZklA = SharedTensor2d(new Tensor2d("Zvector <I|FC>", naoccA, nfrzc));
#pragma omp parallel for
        for (int k = 0; k < naoccA; k++) {
            for (int l = 0; l < nfrzc; l++) {
                double value = FockA->get(k + nfrzc, k + nfrzc) - FockA->get(l, l);
                ZklA->set(k, l, -WorbA->get(k + nfrzc, l) / (2.0 * value));
            }
        }
        ZlkA = SharedTensor2d(new Tensor2d("Zvector <FC|I>", nfrzc, naoccA));
        ZlkA = ZklA->transpose();

        // Beta
        ZklB = SharedTensor2d(new Tensor2d("Zvector <i|FC>", naoccB, nfrzc));
#pragma omp parallel for
        for (int k = 0; k < naoccB; k++) {
            for (int l = 0; l < nfrzc; l++) {
                double value = FockB->get(k + nfrzc, k + nfrzc) - FockB->get(l, l);
                ZklB->set(k, l, -WorbB->get(k + nfrzc, l) / (2.0 * value));
            }
        }
        ZlkB = SharedTensor2d(new Tensor2d("Zvector <FC|i>", nfrzc, naoccB));
        ZlkB = ZklB->transpose();

    }  // end if (reference_ == "UNRESTRICTED")

}  // end z_vector_fc

//=======================================================
//      FC GRAD TERMS
//=======================================================
void DFOCC::fc_grad_terms() {
    SharedTensor2d K, L, IvoA, IvoB, G, Gsep, Z, Z2;
    timer_on("fc_grad_terms");
    if (reference_ == "RESTRICTED") {
        //=========================
        // OPDM
        //=========================
        G1->add_aocc_fc(ZklA, 2.0, 1.0);
        G1->add_fc_aocc(ZlkA, 2.0, 1.0);

        //=========================
        // Seprable TPDM
        //=========================
        // Z_Q' = 4 \sum_{kl} b_{kl}^{Q} Z_kl
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|KL)", nQ_ref, naoccA, nfrzc));
        L->form_b_kl(K);
        K.reset();
        SharedTensor1d Zq = SharedTensor1d(new Tensor1d("DF_BASIS_SCF Zp_Q", nQ_ref));
        Zq->gemv(false, L, ZklA, 4.0, 0.0);
        L.reset();

        // GFM OO Block
        // F_ij += 2 \sum_{Q} b_ij^Q Z_Q'
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
        K->read(psio_, PSIF_DFOCC_INTS);
        GFoo->gemv(true, K, Zq, 2.0, 1.0);
        K.reset();

        // GFM VO Block
        // F_ai += 2 \sum_{Q} b_ai^Q Z_Q'
        // W_ai += 2 \sum_{Q} b_ai^Q Z_Q'
        IvoA = SharedTensor2d(new Tensor2d("MO-basis I <V|O>", nvirA, noccA));
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OV)", nQ_ref, noccA, nvirA));
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|VO)", nQ_ref, nvirA, noccA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L->swap_3index_col(K);
        K.reset();
        IvoA->gemv(true, L, Zq, 2.0, 0.0);
        L.reset();

        // TPDM
        // G_kl^Q += 2 Z_kl J_Q
        // G_lk^Q += 2 Z_kl J_Q
        Gsep = SharedTensor2d(new Tensor2d("3-Index Separable TPDM (Q|OO)", nQ_ref, noccA, noccA));
        Gsep->read(psio_, PSIF_DFOCC_DENS);
#pragma omp parallel for
        for (int Q = 0; Q < nQ_ref; Q++) {
            for (int k = 0; k < naoccA; k++) {
                for (int l = 0; l < nfrzc; l++) {
                    int kl = l + ((k + nfrzc) * noccA);
                    int lk = k + nfrzc + (l * noccA);
                    double value = 2.0 * ZklA->get(k, l) * Jc->get(Q);
                    Gsep->add(Q, kl, value);
                    Gsep->add(Q, lk, value);
                }
            }
        }

//  G_ij^Q += 2 Z_Q \delta_{ij}
#pragma omp parallel for
        for (int Q = 0; Q < nQ_ref; Q++) {
            for (int i = 0; i < noccA; i++) {
                int ii = oo_idxAA->get(i, i);
                Gsep->add(Q, ii, 2.0 * Zq->get(Q));
            }
        }
        Zq.reset();

        // Z_li^Q = 2 * \sum_{k} Z_lk b_ki^Q
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|KI)", nQ_ref, naoccA, noccA));
        L->form_b_ki(K);
        K.reset();
        Z = SharedTensor2d(new Tensor2d("DF_BASIS_SCF Z (Q|LI)", nQ_ref, nfrzc, noccA));
        Z->contract233(false, false, nfrzc, noccA, ZlkA, L, 2.0, 0.0);
        L.reset();

// G_il^Q -= Z_li^Q
// G_li^Q -= Z_li^Q
#pragma omp parallel for
        for (int Q = 0; Q < nQ_ref; Q++) {
            for (int i = 0; i < noccA; i++) {
                for (int l = 0; l < nfrzc; l++) {
                    int il = l + (i * noccA);
                    int li = i + (l * noccA);
                    double value = Z->get(Q, li);
                    Gsep->subtract(Q, il, value);
                    Gsep->subtract(Q, li, value);
                }
            }
        }

        // GFM OO Block
        // F_ij -= \sum_{Q} \sum_{l} b_li^Q Z_lj^Q'
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|LI)", nQ_ref, nfrzc, noccA));
        L->form_b_li(K);
        K.reset();
        GFoo->contract(true, false, noccA, noccA, nQ_ref * nfrzc, L, Z, -1.0, 1.0);
        L.reset();

        // GFM VO Block
        // F_ai -= \sum_{Q} \sum_{l} b_la^Q Z_li^Q'
        // W_ai -= \sum_{Q} \sum_{l} b_la^Q Z_li^Q'
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OV)", nQ_ref, noccA, nvirA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|LA)", nQ_ref, nfrzc, nvirA));
        L->form_b_la(K);
        K.reset();
        IvoA->contract(true, false, nvirA, noccA, nQ_ref * nfrzc, L, Z, -1.0, 1.0);
        L.reset();
        Z.reset();

        // Z_ki^Q = 2 * \sum_{l} Z_kl b_li^Q
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|LI)", nQ_ref, nfrzc, noccA));
        L->form_b_li(K);
        K.reset();
        Z = SharedTensor2d(new Tensor2d("DF_BASIS_SCF Z (Q|KI)", nQ_ref, naoccA, noccA));
        Z->contract233(false, false, naoccA, noccA, ZklA, L, 2.0, 0.0);
        L.reset();

// G_ki^Q -= Z_ki^Q
// G_ik^Q -= Z_ki^Q
#pragma omp parallel for
        for (int Q = 0; Q < nQ_ref; Q++) {
            for (int i = 0; i < noccA; i++) {
                for (int k = 0; k < naoccA; k++) {
                    int ik = k + nfrzc + (i * noccA);
                    int ki = i + ((k + nfrzc) * noccA);
                    int ki2 = i + (k * noccA);
                    double value = Z->get(Q, ki2);
                    Gsep->subtract(Q, ik, value);
                    Gsep->subtract(Q, ki, value);
                }
            }
        }
        Gsep->write(psio_, PSIF_DFOCC_DENS);
        Gsep.reset();

        // GFM OO Block
        // F_ij -= \sum_{Q} \sum_{k} b_ki^Q Z_kj^Q'
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|KI)", nQ_ref, naoccA, noccA));
        L->form_b_ki(K);
        K.reset();
        GFoo->contract(true, false, noccA, noccA, nQ_ref * naoccA, L, Z, -1.0, 1.0);
        L.reset();

        // GFM VO Block
        // F_ai -= \sum_{Q} \sum_{k} b_ka^Q Z_ki^Q'
        // W_ai -= \sum_{Q} \sum_{k} b_ka^Q Z_ki^Q'
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OV)", nQ_ref, noccA, nvirA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|KA)", nQ_ref, naoccA, nvirA));
        L->form_b_ka(K);
        K.reset();
        IvoA->contract(true, false, nvirA, noccA, nQ_ref * naoccA, L, Z, -1.0, 1.0);
        GFvo->add(IvoA);
        WvoA->add(IvoA);
        IvoA.reset();
        L.reset();
        Z.reset();

//=========================
// GFM: AOCC-FC Terms
//=========================
// F_kl += 2.0 * z_kl f_kk
// F_lk += 2.0 * z_kl f_ll
#pragma omp parallel for
        for (int k = 0; k < naoccA; k++) {
            for (int l = 0; l < nfrzc; l++) {
                GFoo->add(k + nfrzc, l, 2.0 * ZklA->get(k, l) * FockA->get(k + nfrzc, k + nfrzc));
                GFoo->add(l, k + nfrzc, 2.0 * ZklA->get(k, l) * FockA->get(l, l));
            }
        }

    }  // end if (reference_ == "RESTRICTED")

    else if (reference_ == "UNRESTRICTED") {
        //=========================
        // OPDM
        //=========================
        G1A->add_aocc_fc(ZklA, 1.0, 1.0);
        G1A->add_fc_aocc(ZlkA, 1.0, 1.0);
        G1B->add_aocc_fc(ZklB, 1.0, 1.0);
        G1B->add_fc_aocc(ZlkB, 1.0, 1.0);

        //=========================
        // Seprable TPDM : Alpha
        //=========================
        // Z_Q' = 2 \sum_{KL} b_{KL}^{Q} Z_KL
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|KL)", nQ_ref, naoccA, nfrzc));
        L->form_b_kl(K);
        K.reset();
        SharedTensor1d Zq = SharedTensor1d(new Tensor1d("DF_BASIS_SCF Zp_Q", nQ_ref));
        Zq->gemv(false, L, ZklA, 2.0, 0.0);
        L.reset();

        // Z_Q' += 2 \sum_{kl} b_{kl}^{Q} Z_kl
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|oo)", nQ_ref, noccB, noccB));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|kl)", nQ_ref, naoccB, nfrzc));
        L->form_b_kl(K);
        K.reset();
        Zq->gemv(false, L, ZklB, 2.0, 1.0);
        L.reset();

        // GFM OO Block
        // F_IJ += \sum_{Q} b_IJ^Q Z_Q'
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
        K->read(psio_, PSIF_DFOCC_INTS);
        GFooA->gemv(true, K, Zq, 1.0, 1.0);
        K.reset();

        // GFM VO Block
        // F_AI += \sum_{Q} b_AI^Q Z_Q'
        // W_AI += \sum_{Q} b_AI^Q Z_Q'
        IvoA = SharedTensor2d(new Tensor2d("MO-basis I <V|O>", nvirA, noccA));
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OV)", nQ_ref, noccA, nvirA));
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|VO)", nQ_ref, nvirA, noccA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L->swap_3index_col(K);
        K.reset();
        IvoA->gemv(true, L, Zq, 1.0, 0.0);
        L.reset();

        // TPDM
        // G_KL^Q += Z_KL J_Q
        // G_LK^Q += Z_KL J_Q
        Gsep = SharedTensor2d(new Tensor2d("3-Index Separable TPDM (Q|OO)", nQ_ref, noccA, noccA));
        Gsep->read(psio_, PSIF_DFOCC_DENS);
#pragma omp parallel for
        for (int Q = 0; Q < nQ_ref; Q++) {
            for (int k = 0; k < naoccA; k++) {
                for (int l = 0; l < nfrzc; l++) {
                    int kl = l + ((k + nfrzc) * noccA);
                    int lk = k + nfrzc + (l * noccA);
                    double value = ZklA->get(k, l) * Jc->get(Q);
                    Gsep->add(Q, kl, value);
                    Gsep->add(Q, lk, value);
                }
            }
        }

//  G_IJ^Q += Z_Q \delta_{IJ}
#pragma omp parallel for
        for (int Q = 0; Q < nQ_ref; Q++) {
            for (int i = 0; i < noccA; i++) {
                int ii = oo_idxAA->get(i, i);
                Gsep->add(Q, ii, Zq->get(Q));
            }
        }
        // Zq.reset();

        // Z_LI^Q = \sum_{K} Z_LK b_KI^Q
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|KI)", nQ_ref, naoccA, noccA));
        L->form_b_ki(K);
        K.reset();
        Z = SharedTensor2d(new Tensor2d("DF_BASIS_SCF Z (Q|LI)", nQ_ref, nfrzc, noccA));
        Z->contract233(false, false, nfrzc, noccA, ZlkA, L, 1.0, 0.0);
        L.reset();

// G_IL^Q -= Z_LI^Q
// G_LI^Q -= Z_LI^Q
#pragma omp parallel for
        for (int Q = 0; Q < nQ_ref; Q++) {
            for (int i = 0; i < noccA; i++) {
                for (int l = 0; l < nfrzc; l++) {
                    int il = l + (i * noccA);
                    int li = i + (l * noccA);
                    double value = Z->get(Q, li);
                    Gsep->subtract(Q, il, value);
                    Gsep->subtract(Q, li, value);
                }
            }
        }

        // GFM OO Block
        // F_IJ -= \sum_{Q} \sum_{L} b_LI^Q Z_LJ^Q'
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|LI)", nQ_ref, nfrzc, noccA));
        L->form_b_li(K);
        K.reset();
        GFooA->contract(true, false, noccA, noccA, nQ_ref * nfrzc, L, Z, -1.0, 1.0);
        L.reset();

        // GFM VO Block
        // F_AI -= \sum_{Q} \sum_{L} b_LA^Q Z_LI^Q'
        // W_AI -= \sum_{Q} \sum_{L} b_LA^Q Z_LI^Q'
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OV)", nQ_ref, noccA, nvirA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|LA)", nQ_ref, nfrzc, nvirA));
        L->form_b_la(K);
        K.reset();
        IvoA->contract(true, false, nvirA, noccA, nQ_ref * nfrzc, L, Z, -1.0, 1.0);
        L.reset();
        Z.reset();

        // Z_KI^Q = \sum_{L} Z_KL b_LI^Q
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|LI)", nQ_ref, nfrzc, noccA));
        L->form_b_li(K);
        K.reset();
        Z = SharedTensor2d(new Tensor2d("DF_BASIS_SCF Z (Q|KI)", nQ_ref, naoccA, noccA));
        Z->contract233(false, false, naoccA, noccA, ZklA, L, 1.0, 0.0);
        L.reset();

// G_KI^Q -= Z_KI^Q
// G_IK^Q -= Z_KI^Q
#pragma omp parallel for
        for (int Q = 0; Q < nQ_ref; Q++) {
            for (int i = 0; i < noccA; i++) {
                for (int k = 0; k < naoccA; k++) {
                    int ik = k + nfrzc + (i * noccA);
                    int ki = i + ((k + nfrzc) * noccA);
                    int ki2 = i + (k * noccA);
                    double value = Z->get(Q, ki2);
                    Gsep->subtract(Q, ik, value);
                    Gsep->subtract(Q, ki, value);
                }
            }
        }
        Gsep->write(psio_, PSIF_DFOCC_DENS);
        Gsep.reset();

        // GFM OO Block
        // F_IJ -= \sum_{Q} \sum_{K} b_KI^Q Z_KJ^Q'
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|KI)", nQ_ref, naoccA, noccA));
        L->form_b_ki(K);
        K.reset();
        GFooA->contract(true, false, noccA, noccA, nQ_ref * naoccA, L, Z, -1.0, 1.0);
        L.reset();

        // GFM VO Block
        // F_AI -= \sum_{Q} \sum_{K} b_KA^Q Z_KI^Q'
        // W_AI -= \sum_{Q} \sum_{k} b_KA^Q Z_KI^Q'
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OV)", nQ_ref, noccA, nvirA));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|KA)", nQ_ref, naoccA, nvirA));
        L->form_b_ka(K);
        K.reset();
        IvoA->contract(true, false, nvirA, noccA, nQ_ref * naoccA, L, Z, -1.0, 1.0);
        L.reset();
        Z.reset();
        GFvoA->add(IvoA);
        WvoA->add(2.0, IvoA);
        IvoA.reset();

//=========================
// GFM: AOCC-FC Terms
//=========================
// F_kl += z_kl f_kk
// F_lk += z_kl f_ll
#pragma omp parallel for
        for (int k = 0; k < naoccA; k++) {
            for (int l = 0; l < nfrzc; l++) {
                GFooA->add(k + nfrzc, l, ZklA->get(k, l) * FockA->get(k + nfrzc, k + nfrzc));
                GFooA->add(l, k + nfrzc, ZklA->get(k, l) * FockA->get(l, l));
            }
        }

        //=========================
        // Seprable TPDM : Beta
        //=========================
        // GFM oo Block
        // F_ij += \sum_{Q} b_ij^Q Z_Q'
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|oo)", nQ_ref, noccB, noccB));
        K->read(psio_, PSIF_DFOCC_INTS);
        GFooB->gemv(true, K, Zq, 1.0, 1.0);
        K.reset();

        // GFM vo Block
        // F_ai += \sum_{Q} b_ai^Q Z_Q'
        // W_ai += \sum_{Q} b_ai^Q Z_Q'
        IvoB = SharedTensor2d(new Tensor2d("MO-basis I <v|o>", nvirB, noccB));
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|ov)", nQ_ref, noccB, nvirB));
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|vo)", nQ_ref, nvirB, noccB));
        K->read(psio_, PSIF_DFOCC_INTS);
        L->swap_3index_col(K);
        K.reset();
        IvoB->gemv(true, L, Zq, 1.0, 0.0);
        L.reset();

        // TPDM
        // G_kl^Q += Z_kl J_Q
        // G_lk^Q += Z_kl J_Q
        Gsep = SharedTensor2d(new Tensor2d("3-Index Separable TPDM (Q|oo)", nQ_ref, noccB, noccB));
        Gsep->read(psio_, PSIF_DFOCC_DENS);
#pragma omp parallel for
        for (int Q = 0; Q < nQ_ref; Q++) {
            for (int k = 0; k < naoccB; k++) {
                for (int l = 0; l < nfrzc; l++) {
                    int kl = l + ((k + nfrzc) * noccB);
                    int lk = k + nfrzc + (l * noccB);
                    double value = ZklB->get(k, l) * Jc->get(Q);
                    Gsep->add(Q, kl, value);
                    Gsep->add(Q, lk, value);
                }
            }
        }

//  G_ij^Q += Z_Q \delta_{ij}
#pragma omp parallel for
        for (int Q = 0; Q < nQ_ref; Q++) {
            for (int i = 0; i < noccB; i++) {
                int ii = oo_idxBB->get(i, i);
                Gsep->add(Q, ii, Zq->get(Q));
            }
        }
        Zq.reset();

        // Z_li^Q = \sum_{k} Z_lk b_ki^Q
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|oo)", nQ_ref, noccB, noccB));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|ki)", nQ_ref, naoccB, noccB));
        L->form_b_ki(K);
        K.reset();
        Z = SharedTensor2d(new Tensor2d("DF_BASIS_SCF Z (Q|li)", nQ_ref, nfrzc, noccB));
        Z->contract233(false, false, nfrzc, noccB, ZlkB, L, 1.0, 0.0);
        L.reset();

// G_il^Q -= Z_li^Q
// G_li^Q -= Z_li^Q
#pragma omp parallel for
        for (int Q = 0; Q < nQ_ref; Q++) {
            for (int i = 0; i < noccB; i++) {
                for (int l = 0; l < nfrzc; l++) {
                    int il = l + (i * noccB);
                    int li = i + (l * noccB);
                    double value = Z->get(Q, li);
                    Gsep->subtract(Q, il, value);
                    Gsep->subtract(Q, li, value);
                }
            }
        }

        // GFM oo Block
        // F_ij -= \sum_{Q} \sum_{l} b_li^Q Z_lj^Q'
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|oo)", nQ_ref, noccB, noccB));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|li)", nQ_ref, nfrzc, noccB));
        L->form_b_li(K);
        K.reset();
        GFooB->contract(true, false, noccB, noccB, nQ_ref * nfrzc, L, Z, -1.0, 1.0);
        L.reset();

        // GFM vo Block
        // F_ai -= \sum_{Q} \sum_{l} b_la^Q Z_li^Q'
        // W_ai -= \sum_{Q} \sum_{l} b_la^Q Z_li^Q'
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|ov)", nQ_ref, noccB, nvirB));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|la)", nQ_ref, nfrzc, nvirB));
        L->form_b_la(K);
        K.reset();
        IvoB->contract(true, false, nvirB, noccB, nQ_ref * nfrzc, L, Z, -1.0, 1.0);
        L.reset();
        Z.reset();

        // Z_ki^Q = \sum_{l} Z_kl b_li^Q
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|oo)", nQ_ref, noccB, noccB));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|li)", nQ_ref, nfrzc, noccB));
        L->form_b_li(K);
        K.reset();
        Z = SharedTensor2d(new Tensor2d("DF_BASIS_SCF Z (Q|ki)", nQ_ref, naoccB, noccB));
        Z->contract233(false, false, naoccB, noccB, ZklB, L, 1.0, 0.0);
        L.reset();

// G_ki^Q -= Z_ki^Q
// G_ik^Q -= Z_ki^Q
#pragma omp parallel for
        for (int Q = 0; Q < nQ_ref; Q++) {
            for (int i = 0; i < noccB; i++) {
                for (int k = 0; k < naoccB; k++) {
                    int ik = k + nfrzc + (i * noccB);
                    int ki = i + ((k + nfrzc) * noccB);
                    int ki2 = i + (k * noccB);
                    double value = Z->get(Q, ki2);
                    Gsep->subtract(Q, ik, value);
                    Gsep->subtract(Q, ki, value);
                }
            }
        }
        Gsep->write(psio_, PSIF_DFOCC_DENS);
        Gsep.reset();

        // GFM oo Block
        // F_ij -= \sum_{Q} \sum_{k} b_ki^Q Z_kj^Q'
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|oo)", nQ_ref, noccB, noccB));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|ki)", nQ_ref, naoccB, noccB));
        L->form_b_ki(K);
        K.reset();
        GFooB->contract(true, false, noccB, noccB, nQ_ref * naoccB, L, Z, -1.0, 1.0);
        L.reset();

        // GFM VO Block
        // F_ai -= \sum_{Q} \sum_{k} b_ka^Q Z_ki^Q'
        // W_ai -= \sum_{Q} \sum_{k} b_ka^Q Z_ki^Q'
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|ov)", nQ_ref, noccB, nvirB));
        K->read(psio_, PSIF_DFOCC_INTS);
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|ka)", nQ_ref, naoccB, nvirB));
        L->form_b_ka(K);
        K.reset();
        IvoB->contract(true, false, nvirB, noccB, nQ_ref * naoccB, L, Z, -1.0, 1.0);
        L.reset();
        Z.reset();
        GFvoB->add(IvoB);
        WvoB->add(2.0, IvoB);
        IvoB.reset();

//=========================
// GFM: AOCC-FC Terms
//=========================
// F_kl += z_kl f_kk
// F_lk += z_kl f_ll
#pragma omp parallel for
        for (int k = 0; k < naoccB; k++) {
            for (int l = 0; l < nfrzc; l++) {
                GFooB->add(k + nfrzc, l, ZklB->get(k, l) * FockB->get(k + nfrzc, k + nfrzc));
                GFooB->add(l, k + nfrzc, ZklB->get(k, l) * FockB->get(l, l));
            }
        }

    }  // end if (reference_ == "UNRESTRICTED")
    timer_off("fc_grad_terms");

}  // end fc_grad_terms

//=======================================================
//      OO GRAD TERMS
//=======================================================
void DFOCC::oo_grad_terms() {
    SharedTensor2d K, L, M, IvoA, IvoB, G, Gsep, Z, Z2;
    timer_on("oo_grad_terms");
    if (reference_ == "RESTRICTED") {
        //=========================
        // OPDM
        //=========================
        G1->add_oo(ZklA, 2.0, 1.0);

        //=========================
        // Seprable TPDM
        //=========================
        // G_ij^Q += 2 Z_ij J_Q
        Gsep = SharedTensor2d(new Tensor2d("3-Index Separable TPDM (Q|OO)", nQ_ref, noccA, noccA));
        Gsep->read(psio_, PSIF_DFOCC_DENS);
        Gsep->dirprd123(Jc, ZklA, 2.0, 1.0);

        // Read OO Ints
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
        K->read(psio_, PSIF_DFOCC_INTS);

        // Z_Q' = 2 \sum_{kl} b_{kl}^{Q} Z_kl
        SharedTensor1d Zq = SharedTensor1d(new Tensor1d("DF_BASIS_SCF Zp_Q", nQ_ref));
        Zq->gemv(false, K, ZklA, 2.0, 0.0);

//  G_ij^Q += 2 Z_Q' \delta_{ij}
#pragma omp parallel for
        for (int Q = 0; Q < nQ_ref; Q++) {
            for (int i = 0; i < noccA; i++) {
                int ii = oo_idxAA->get(i, i);
                Gsep->add(Q, ii, 2.0 * Zq->get(Q));
            }
        }

        // Z_ij^Q' = 4 * \sum_{k} Z_ik b_kj^Q
        Z = SharedTensor2d(new Tensor2d("DF_BASIS_SCF Z (Q|IJ)", nQ_ref, noccA, noccA));
        Z->contract233(false, false, noccA, noccA, ZklA, K, 4.0, 0.0);

        // G_ij^Q -= Z_ij^Q'
        Gsep->axpy(Z, -1.0);
        Gsep->write(psio_, PSIF_DFOCC_DENS);
        Gsep.reset();

//=========================
// GFM
//=========================
// GFM OO Block
// F_ij += 2.0 * z_ij f_ii
#pragma omp parallel for
        for (int k = 0; k < noccA; k++) {
            for (int l = 0; l < noccA; l++) {
                GFoo->add(k, l, 2.0 * ZklA->get(k, l) * FockA->get(k, k));
            }
        }

        // F_ij += 2 \sum_{Q} b_ij^Q Z_Q'
        GFoo->gemv(true, K, Zq, 2.0, 1.0);

        // F_ij -= 1/2\sum_{Q} \sum_{l} b_li^Q Z_lj^Q'
        GFoo->contract(true, false, noccA, noccA, nQ_ref * noccA, K, Z, -0.5, 1.0);

        // GFM VO Block
        // F_ai += 2 \sum_{Q} b_ai^Q Z_Q'
        // W_ai += 2 \sum_{Q} b_ai^Q Z_Q'
        IvoA = SharedTensor2d(new Tensor2d("MO-basis I <V|O>", nvirA, noccA));
        M = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OV)", nQ_ref, noccA, nvirA));
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|VO)", nQ_ref, nvirA, noccA));
        M->read(psio_, PSIF_DFOCC_INTS);
        L->swap_3index_col(M);
        IvoA->gemv(true, L, Zq, 2.0, 0.0);
        L.reset();

        // F_ai -= 1/2 \sum_{Q} \sum_{l} b_la^Q Z_li^Q'
        // W_ai -= 1/2 \sum_{Q} \sum_{l} b_la^Q Z_li^Q'
        IvoA->contract(true, false, nvirA, noccA, nQ_ref * noccA, M, Z, -0.5, 1.0);
        GFvo->add(IvoA);
        WvoA->add(IvoA);
        IvoA.reset();
        M.reset();

        // Clean
        Zq.reset();
        K.reset();
        Z.reset();
        // outfile->Printf("\tOO grad is done.\n");

    }  // end if (reference_ == "RESTRICTED")

    else if (reference_ == "UNRESTRICTED") {
        /*
            //=========================
            // OPDM
            //=========================
            G1A->add_aocc_fc(ZklA, 1.0, 1.0);
            G1A->add_fc_aocc(ZlkA, 1.0, 1.0);
            G1B->add_aocc_fc(ZklB, 1.0, 1.0);
            G1B->add_fc_aocc(ZlkB, 1.0, 1.0);

            //=========================
            // Seprable TPDM : Alpha
            //=========================
            // Z_Q' = 2 \sum_{KL} b_{KL}^{Q} Z_KL
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
            K->read(psio_, PSIF_DFOCC_INTS);
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|KL)", nQ_ref, naoccA, nfrzc));
            L->form_b_kl(K);
            K.reset();
            SharedTensor1d Zq = SharedTensor1d(new Tensor1d("DF_BASIS_SCF Zp_Q", nQ_ref));
            Zq->gemv(false, L, ZklA, 2.0, 0.0);
            L.reset();

            // Z_Q' += 2 \sum_{kl} b_{kl}^{Q} Z_kl
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|oo)", nQ_ref, noccB, noccB));
            K->read(psio_, PSIF_DFOCC_INTS);
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|kl)", nQ_ref, naoccB, nfrzc));
            L->form_b_kl(K);
            K.reset();
            Zq->gemv(false, L, ZklB, 2.0, 1.0);
            L.reset();

            // GFM OO Block
            // F_IJ += \sum_{Q} b_IJ^Q Z_Q'
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
            K->read(psio_, PSIF_DFOCC_INTS);
            GFooA->gemv(true, K, Zq, 1.0, 1.0);
            K.reset();

            // GFM VO Block
            // F_AI += \sum_{Q} b_AI^Q Z_Q'
            // W_AI += \sum_{Q} b_AI^Q Z_Q'
            IvoA = SharedTensor2d(new Tensor2d("MO-basis I <V|O>", nvirA, noccA));
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OV)", nQ_ref, noccA, nvirA));
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|VO)", nQ_ref, nvirA, noccA));
            K->read(psio_, PSIF_DFOCC_INTS);
            L->swap_3index_col(K);
            K.reset();
            IvoA->gemv(true, L, Zq, 1.0, 0.0);
            L.reset();

            // TPDM
            // G_KL^Q += Z_KL J_Q
            // G_LK^Q += Z_KL J_Q
            Gsep = SharedTensor2d(new Tensor2d("3-Index Separable TPDM (Q|OO)", nQ_ref, noccA, noccA));
            Gsep->read(psio_, PSIF_DFOCC_DENS);
            #pragma omp parallel for
            for (int Q = 0; Q < nQ_ref; Q++) {
                 for (int k = 0; k < naoccA; k++) {
                      for (int l = 0; l < nfrzc; l++) {
                           int kl = l + ( (k + nfrzc) * noccA);
                           int lk = k + nfrzc + (l*noccA);
                           double value = ZklA->get(k,l) * Jc->get(Q);
                           Gsep->add(Q, kl, value);
                           Gsep->add(Q, lk, value);
                      }
                 }
            }

            //  G_IJ^Q += Z_Q \delta_{IJ}
            #pragma omp parallel for
            for (int Q = 0; Q < nQ_ref; Q++) {
                 for (int i = 0; i < noccA; i++) {
                      int ii = oo_idxAA->get(i,i);
                      Gsep->add(Q, ii, Zq->get(Q));
                 }
            }
            //Zq.reset();

            // Z_LI^Q = \sum_{K} Z_LK b_KI^Q
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
            K->read(psio_, PSIF_DFOCC_INTS);
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|KI)", nQ_ref, naoccA, noccA));
            L->form_b_ki(K);
            K.reset();
            Z = SharedTensor2d(new Tensor2d("DF_BASIS_SCF Z (Q|LI)", nQ_ref, nfrzc, noccA));
            Z->contract233(false, false, nfrzc, noccA, ZlkA, L, 1.0, 0.0);
            L.reset();

            // G_IL^Q -= Z_LI^Q
            // G_LI^Q -= Z_LI^Q
            #pragma omp parallel for
            for (int Q = 0; Q < nQ_ref; Q++) {
                 for (int i = 0; i < noccA; i++) {
                      for (int l = 0; l < nfrzc; l++) {
                           int il = l + (i*noccA);
                           int li = i + (l*noccA);
                           double value = Z->get(Q, li);
                           Gsep->subtract(Q, il, value);
                           Gsep->subtract(Q, li, value);
                      }
                 }
            }

            // GFM OO Block
            // F_IJ -= \sum_{Q} \sum_{L} b_LI^Q Z_LJ^Q'
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
            K->read(psio_, PSIF_DFOCC_INTS);
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|LI)", nQ_ref, nfrzc, noccA));
            L->form_b_li(K);
            K.reset();
            GFooA->contract(true, false, noccA, noccA, nQ_ref * nfrzc, L, Z, -1.0, 1.0);
            L.reset();

            // GFM VO Block
            // F_AI -= \sum_{Q} \sum_{L} b_LA^Q Z_LI^Q'
            // W_AI -= \sum_{Q} \sum_{L} b_LA^Q Z_LI^Q'
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OV)", nQ_ref, noccA, nvirA));
            K->read(psio_, PSIF_DFOCC_INTS);
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|LA)", nQ_ref, nfrzc, nvirA));
            L->form_b_la(K);
            K.reset();
            IvoA->contract(true, false, nvirA, noccA, nQ_ref * nfrzc, L, Z, -1.0, 1.0);
            L.reset();
            Z.reset();

            // Z_KI^Q = \sum_{L} Z_KL b_LI^Q
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
            K->read(psio_, PSIF_DFOCC_INTS);
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|LI)", nQ_ref, nfrzc, noccA));
            L->form_b_li(K);
            K.reset();
            Z = SharedTensor2d(new Tensor2d("DF_BASIS_SCF Z (Q|KI)", nQ_ref, naoccA, noccA));
            Z->contract233(false, false, naoccA, noccA, ZklA, L, 1.0, 0.0);
            L.reset();

            // G_KI^Q -= Z_KI^Q
            // G_IK^Q -= Z_KI^Q
            #pragma omp parallel for
            for (int Q = 0; Q < nQ_ref; Q++) {
                 for (int i = 0; i < noccA; i++) {
                      for (int k = 0; k < naoccA; k++) {
                           int ik = k + nfrzc + (i*noccA);
                           int ki = i + ( (k+nfrzc) * noccA);
                           int ki2 = i + (k*noccA);
                           double value = Z->get(Q, ki2);
                           Gsep->subtract(Q, ik, value);
                           Gsep->subtract(Q, ki, value);
                      }
                 }
            }
            Gsep->write(psio_, PSIF_DFOCC_DENS);
            Gsep.reset();

            // GFM OO Block
            // F_IJ -= \sum_{Q} \sum_{K} b_KI^Q Z_KJ^Q'
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
            K->read(psio_, PSIF_DFOCC_INTS);
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|KI)", nQ_ref, naoccA, noccA));
            L->form_b_ki(K);
            K.reset();
            GFooA->contract(true, false, noccA, noccA, nQ_ref * naoccA, L, Z, -1.0, 1.0);
            L.reset();

            // GFM VO Block
            // F_AI -= \sum_{Q} \sum_{K} b_KA^Q Z_KI^Q'
            // W_AI -= \sum_{Q} \sum_{k} b_KA^Q Z_KI^Q'
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OV)", nQ_ref, noccA, nvirA));
            K->read(psio_, PSIF_DFOCC_INTS);
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|KA)", nQ_ref, naoccA, nvirA));
            L->form_b_ka(K);
            K.reset();
            IvoA->contract(true, false, nvirA, noccA, nQ_ref * naoccA, L, Z, -1.0, 1.0);
            L.reset();
            Z.reset();
            GFvoA->add(IvoA);
            WvoA->add(2.0, IvoA);
            IvoA.reset();

            //=========================
            // GFM: AOCC-FC Terms
            //=========================
            // F_kl += z_kl f_kk
            // F_lk += z_kl f_ll
            #pragma omp parallel for
            for (int k = 0; k < naoccA; k++) {
                 for (int l = 0; l < nfrzc; l++) {
                      GFooA->add(k + nfrzc, l, ZklA->get(k,l) * FockA->get(k + nfrzc, k + nfrzc));
                      GFooA->add(l, k + nfrzc, ZklA->get(k,l) * FockA->get(l, l));
                 }
            }

            //=========================
            // Seprable TPDM : Beta
            //=========================
            // GFM oo Block
            // F_ij += \sum_{Q} b_ij^Q Z_Q'
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|oo)", nQ_ref, noccB, noccB));
            K->read(psio_, PSIF_DFOCC_INTS);
            GFooB->gemv(true, K, Zq, 1.0, 1.0);
            K.reset();

            // GFM vo Block
            // F_ai += \sum_{Q} b_ai^Q Z_Q'
            // W_ai += \sum_{Q} b_ai^Q Z_Q'
            IvoB = SharedTensor2d(new Tensor2d("MO-basis I <v|o>", nvirB, noccB));
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|ov)", nQ_ref, noccB, nvirB));
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|vo)", nQ_ref, nvirB, noccB));
            K->read(psio_, PSIF_DFOCC_INTS);
            L->swap_3index_col(K);
            K.reset();
            IvoB->gemv(true, L, Zq, 1.0, 0.0);
            L.reset();

            // TPDM
            // G_kl^Q += Z_kl J_Q
            // G_lk^Q += Z_kl J_Q
            Gsep = SharedTensor2d(new Tensor2d("3-Index Separable TPDM (Q|oo)", nQ_ref, noccB, noccB));
            Gsep->read(psio_, PSIF_DFOCC_DENS);
            #pragma omp parallel for
            for (int Q = 0; Q < nQ_ref; Q++) {
                 for (int k = 0; k < naoccB; k++) {
                      for (int l = 0; l < nfrzc; l++) {
                           int kl = l + ( (k + nfrzc) * noccB);
                           int lk = k + nfrzc + (l*noccB);
                           double value = ZklB->get(k,l) * Jc->get(Q);
                           Gsep->add(Q, kl, value);
                           Gsep->add(Q, lk, value);
                      }
                 }
            }

            //  G_ij^Q += Z_Q \delta_{ij}
            #pragma omp parallel for
            for (int Q = 0; Q < nQ_ref; Q++) {
                 for (int i = 0; i < noccB; i++) {
                      int ii = oo_idxBB->get(i,i);
                      Gsep->add(Q, ii, Zq->get(Q));
                 }
            }
            Zq.reset();

            // Z_li^Q = \sum_{k} Z_lk b_ki^Q
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|oo)", nQ_ref, noccB, noccB));
            K->read(psio_, PSIF_DFOCC_INTS);
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|ki)", nQ_ref, naoccB, noccB));
            L->form_b_ki(K);
            K.reset();
            Z = SharedTensor2d(new Tensor2d("DF_BASIS_SCF Z (Q|li)", nQ_ref, nfrzc, noccB));
            Z->contract233(false, false, nfrzc, noccB, ZlkB, L, 1.0, 0.0);
            L.reset();

            // G_il^Q -= Z_li^Q
            // G_li^Q -= Z_li^Q
            #pragma omp parallel for
            for (int Q = 0; Q < nQ_ref; Q++) {
                 for (int i = 0; i < noccB; i++) {
                      for (int l = 0; l < nfrzc; l++) {
                           int il = l + (i*noccB);
                           int li = i + (l*noccB);
                           double value = Z->get(Q, li);
                           Gsep->subtract(Q, il, value);
                           Gsep->subtract(Q, li, value);
                      }
                 }
            }

            // GFM oo Block
            // F_ij -= \sum_{Q} \sum_{l} b_li^Q Z_lj^Q'
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|oo)", nQ_ref, noccB, noccB));
            K->read(psio_, PSIF_DFOCC_INTS);
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|li)", nQ_ref, nfrzc, noccB));
            L->form_b_li(K);
            K.reset();
            GFooB->contract(true, false, noccB, noccB, nQ_ref * nfrzc, L, Z, -1.0, 1.0);
            L.reset();

            // GFM vo Block
            // F_ai -= \sum_{Q} \sum_{l} b_la^Q Z_li^Q'
            // W_ai -= \sum_{Q} \sum_{l} b_la^Q Z_li^Q'
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|ov)", nQ_ref, noccB, nvirB));
            K->read(psio_, PSIF_DFOCC_INTS);
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|la)", nQ_ref, nfrzc, nvirB));
            L->form_b_la(K);
            K.reset();
            IvoB->contract(true, false, nvirB, noccB, nQ_ref * nfrzc, L, Z, -1.0, 1.0);
            L.reset();
            Z.reset();

            // Z_ki^Q = \sum_{l} Z_kl b_li^Q
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|oo)", nQ_ref, noccB, noccB));
            K->read(psio_, PSIF_DFOCC_INTS);
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|li)", nQ_ref, nfrzc, noccB));
            L->form_b_li(K);
            K.reset();
            Z = SharedTensor2d(new Tensor2d("DF_BASIS_SCF Z (Q|ki)", nQ_ref, naoccB, noccB));
            Z->contract233(false, false, naoccB, noccB, ZklB, L, 1.0, 0.0);
            L.reset();

            // G_ki^Q -= Z_ki^Q
            // G_ik^Q -= Z_ki^Q
            #pragma omp parallel for
            for (int Q = 0; Q < nQ_ref; Q++) {
                 for (int i = 0; i < noccB; i++) {
                      for (int k = 0; k < naoccB; k++) {
                           int ik = k + nfrzc + (i*noccB);
                           int ki = i + ( (k+nfrzc) * noccB);
                           int ki2 = i + (k*noccB);
                           double value = Z->get(Q, ki2);
                           Gsep->subtract(Q, ik, value);
                           Gsep->subtract(Q, ki, value);
                      }
                 }
            }
            Gsep->write(psio_, PSIF_DFOCC_DENS);
            Gsep.reset();

            // GFM oo Block
            // F_ij -= \sum_{Q} \sum_{k} b_ki^Q Z_kj^Q'
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|oo)", nQ_ref, noccB, noccB));
            K->read(psio_, PSIF_DFOCC_INTS);
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|ki)", nQ_ref, naoccB, noccB));
            L->form_b_ki(K);
            K.reset();
            GFooB->contract(true, false, noccB, noccB, nQ_ref * naoccB, L, Z, -1.0, 1.0);
            L.reset();

            // GFM VO Block
            // F_ai -= \sum_{Q} \sum_{k} b_ka^Q Z_ki^Q'
            // W_ai -= \sum_{Q} \sum_{k} b_ka^Q Z_ki^Q'
            K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|ov)", nQ_ref, noccB, nvirB));
            K->read(psio_, PSIF_DFOCC_INTS);
            L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|ka)", nQ_ref, naoccB, nvirB));
            L->form_b_ka(K);
            K.reset();
            IvoB->contract(true, false, nvirB, noccB, nQ_ref * naoccB, L, Z, -1.0, 1.0);
            L.reset();
            Z.reset();
            GFvoB->add(IvoB);
            WvoB->add(2.0, IvoB);
            IvoB.reset();

            //=========================
            // GFM: AOCC-FC Terms
            //=========================
            // F_kl += z_kl f_kk
            // F_lk += z_kl f_ll
            #pragma omp parallel for
            for (int k = 0; k < naoccB; k++) {
                 for (int l = 0; l < nfrzc; l++) {
                      GFooB->add(k + nfrzc, l, ZklB->get(k,l) * FockB->get(k + nfrzc, k + nfrzc));
                      GFooB->add(l, k + nfrzc, ZklB->get(k,l) * FockB->get(l, l));
                 }
            }
        */
    }  // end if (reference_ == "UNRESTRICTED")
    timer_off("oo_grad_terms");

}  // end oo_grad_terms

//=======================================================
//      VV GRAD TERMS
//=======================================================
void DFOCC::vv_grad_terms() {
    SharedTensor2d K, L, M, IvoA, IvoB, G, Gsep, Z, Z2;
    timer_on("vv_grad_terms");
    if (reference_ == "RESTRICTED") {
        //=========================
        // OPDM
        //=========================
        G1->add_vv(noccA, ZcdA, 2.0, 1.0);

        //=========================
        // Seprable TPDM
        //=========================
        // TPDM OO Block
        // Z_Q'' = 2 \sum_{cd} b_{cd}^{Q} Z_cd
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|VV)", nQ_ref, nvirA, nvirA));
        K->read(psio_, PSIF_DFOCC_INTS, true, true);
        SharedTensor1d Zq = SharedTensor1d(new Tensor1d("DF_BASIS_SCF Zp_Q", nQ_ref));
        Zq->gemv(false, K, ZcdA, 2.0, 0.0);
        K.reset();

        //  G_ij^Q += 2 Z_Q'' \delta_{ij}
        Gsep = SharedTensor2d(new Tensor2d("3-Index Separable TPDM (Q|OO)", nQ_ref, noccA, noccA));
        Gsep->read(psio_, PSIF_DFOCC_DENS);
#pragma omp parallel for
        for (int Q = 0; Q < nQ_ref; Q++) {
            for (int i = 0; i < noccA; i++) {
                int ii = oo_idxAA->get(i, i);
                Gsep->add(Q, ii, 2.0 * Zq->get(Q));
            }
        }
        Gsep->write(psio_, PSIF_DFOCC_DENS);
        Gsep.reset();
        // outfile->Printf("\tTPDM:OO is done.\n");

        // TPDM VV Block
        // G_ab^Q += 2 Z_ab J_Q
        Gsep = SharedTensor2d(new Tensor2d("3-Index Separable TPDM (Q|VV)", nQ_ref, nvirA, nvirA));
        Gsep->read(psio_, PSIF_DFOCC_DENS, true, true);
        Gsep->dirprd123(Jc, ZcdA, 2.0, 1.0);
        Gsep->write(psio_, PSIF_DFOCC_DENS, true, true);
        Gsep.reset();
        // outfile->Printf("\tTPDM:VV is done.\n");

        // TPDM OV Block
        // Read OV Ints
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OV)", nQ_ref, noccA, nvirA));
        L->read(psio_, PSIF_DFOCC_INTS);
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|VO)", nQ_ref, nvirA, noccA));
        K->swap_3index_col(L);
        L.reset();

        // Z_ai^Q'' = 2 * \sum_{b} Z_ab b_bi^Q
        Z = SharedTensor2d(new Tensor2d("DF_BASIS_SCF Z (Q|AI)", nQ_ref, nvirA, noccA));
        Z->contract233(false, false, nvirA, noccA, ZcdA, K, 2.0, 0.0);

        // G_ia^Q -= Z_ai^Q''
        // G_ai^Q -= Z_ai^Q''
        Gsep = SharedTensor2d(new Tensor2d("3-Index Separable TPDM (Q|OV)", nQ_ref, noccA, nvirA));
        G = SharedTensor2d(new Tensor2d("3-Index Separable TPDM (Q|VO)", nQ_ref, nvirA, noccA));
        Gsep->read(psio_, PSIF_DFOCC_DENS);
        G->read(psio_, PSIF_DFOCC_DENS);
#pragma omp parallel for
        for (int Q = 0; Q < nQ_ref; Q++) {
            for (int i = 0; i < noccA; i++) {
                for (int a = 0; a < nvirA; a++) {
                    int ia = ov_idxAA->get(i, a);
                    int ai = vo_idxAA->get(a, i);
                    double value = Z->get(Q, ai);
                    Gsep->subtract(Q, ia, value);
                    G->subtract(Q, ai, value);
                }
            }
        }
        Gsep->write(psio_, PSIF_DFOCC_DENS);
        G->write(psio_, PSIF_DFOCC_DENS);
        Gsep.reset();
        G.reset();

        //=========================
        // GFM
        //=========================
        // GFM OO Block
        // F_ij += 2 \sum_{Q} b_ij^Q Z_Q''
        L = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|OO)", nQ_ref, noccA, noccA));
        L->read(psio_, PSIF_DFOCC_INTS);
        GFoo->gemv(true, L, Zq, 2.0, 1.0);
        L.reset();

        // F_ij -= \sum_{Q} \sum_{d} b_di^Q Z_dj^Q''
        GFoo->contract(true, false, noccA, noccA, nQ_ref * nvirA, K, Z, -1.0, 1.0);

// GFM VV Block
// F_ab += 2.0 * z_ab f_a
#pragma omp parallel for
        for (int c = 0; c < nvirA; c++) {
            for (int d = 0; d < nvirA; d++) {
                GFvv->add(c, d, 2.0 * ZcdA->get(c, d) * FockA->get(c + noccA, c + noccA));
            }
        }
        GF->set_vv(noccA, GFvv);
        // outfile->Printf("\tGFM:VV is done.\n");

        // GFM VO Block
        // F_ai += 2 \sum_{Q} b_ai^Q Z_Q''
        // W_ai += 2 \sum_{Q} b_ai^Q Z_Q''
        IvoA = SharedTensor2d(new Tensor2d("MO-basis I <V|O>", nvirA, noccA));
        IvoA->gemv(true, K, Zq, 2.0, 0.0);

        // GFM VO Block
        // F_ai -= \sum_{Q} \sum_{d} b_da^Q Z_di^Q''
        // w_ai -= \sum_{Q} \sum_{d} b_da^Q Z_di^Q''
        K.reset();
        K = SharedTensor2d(new Tensor2d("DF_BASIS_SCF B (Q|VV)", nQ_ref, nvirA, nvirA));
        K->read(psio_, PSIF_DFOCC_INTS, true, true);
        IvoA->contract(true, false, nvirA, noccA, nQ_ref * nvirA, K, Z, -1.0, 1.0);
        GFvo->add(IvoA);
        WvoA->add(IvoA);
        IvoA.reset();

        // Clean
        Zq.reset();
        K.reset();
        Z.reset();

    }  // end if (reference_ == "RESTRICTED")

    else if (reference_ == "UNRESTRICTED") {
    }  // end if (reference_ == "UNRESTRICTED")
    timer_off("vv_grad_terms");
}  // end vv_grad_terms

}  // namespace dfoccwave
}  // namespace psi
