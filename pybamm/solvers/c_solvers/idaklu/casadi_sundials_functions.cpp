#include "casadi_sundials_functions.hpp"
#include "casadi_functions.hpp"

int residual_casadi(realtype tres, N_Vector yy, N_Vector yp, N_Vector rr,
                    void *user_data)
{
  CasadiFunctions *p_python_functions =
      static_cast<CasadiFunctions *>(user_data);

  // std::cout << "RESIDUAL t = " << tres << " y = [";
  // for (int i = 0; i < p_python_functions->number_of_states; i++) {
  //   std::cout << NV_DATA_S(yy)[i] << " ";
  // }
  // std::cout << "] yp = [";
  // for (int i = 0; i < p_python_functions->number_of_states; i++) {
  //   std::cout << NV_DATA_S(yp)[i] << " ";
  // }
  // std::cout << "]" << std::endl;
  //   args are t, y, put result in rr

  p_python_functions->rhs_alg.m_arg[0] = &tres;
  p_python_functions->rhs_alg.m_arg[1] = NV_DATA_S(yy);
  p_python_functions->rhs_alg.m_arg[2] = p_python_functions->inputs.data();
  p_python_functions->rhs_alg.m_res[0] = NV_DATA_S(rr);
  p_python_functions->rhs_alg();

  // std::cout << "rhs_alg = [";
  // for (int i = 0; i < p_python_functions->number_of_states; i++) {
  //   std::cout << NV_DATA_S(rr)[i] << " ";
  // }
  // std::cout << "]" << std::endl;

  realtype *tmp = p_python_functions->get_tmp();
  // std::cout << "tmp before = [";
  // for (int i = 0; i < p_python_functions->number_of_states; i++) {
  //   std::cout << tmp[i] << " ";
  // }
  // std::cout << "]" << std::endl;
  //  args is yp, put result in tmp
  p_python_functions->mass_action.m_arg[0] = NV_DATA_S(yp);
  p_python_functions->mass_action.m_res[0] = tmp;
  p_python_functions->mass_action();

  // std::cout << "tmp = [";
  // for (int i = 0; i < p_python_functions->number_of_states; i++) {
  //   std::cout << tmp[i] << " ";
  // }
  // std::cout << "]" << std::endl;

  // AXPY: y <- a*x + y
  const int ns = p_python_functions->number_of_states;
  casadi::casadi_axpy(ns, -1., tmp, NV_DATA_S(rr));

  // std::cout << "residual = [";
  // for (int i = 0; i < p_python_functions->number_of_states; i++) {
  //   std::cout << NV_DATA_S(rr)[i] << " ";
  // }
  // std::cout << "]" << std::endl;

  // now rr has rhs_alg(t, y) - mass_matrix * yp

  return 0;
}

// Purpose This function computes the product Jv of the DAE system Jacobian J
// (or an approximation to it) and a given vector v, where J is defined by Eq.
// (2.6).
//    J = ∂F/∂y + cj ∂F/∂y˙
// Arguments tt is the current value of the independent variable.
//     yy is the current value of the dependent variable vector, y(t).
//     yp is the current value of ˙y(t).
//     rr is the current value of the residual vector F(t, y, y˙).
//     v is the vector by which the Jacobian must be multiplied to the right.
//     Jv is the computed output vector.
//     cj is the scalar in the system Jacobian, proportional to the inverse of
//     the step
//        size (α in Eq. (2.6) ).
//     user data is a pointer to user data, the same as the user data parameter
//     passed to
//        IDASetUserData.
//     tmp1
//     tmp2 are pointers to memory allocated for variables of type N Vector
//     which can
//        be used by IDALsJacTimesVecFn as temporary storage or work space.
int jtimes_casadi(realtype tt, N_Vector yy, N_Vector yp, N_Vector rr,
                  N_Vector v, N_Vector Jv, realtype cj, void *user_data,
                  N_Vector tmp1, N_Vector tmp2)
{
  CasadiFunctions *p_python_functions =
      static_cast<CasadiFunctions *>(user_data);

  // rr has ∂F/∂y v
  p_python_functions->jac_action.m_arg[0] = &tt;
  p_python_functions->jac_action.m_arg[1] = NV_DATA_S(yy);
  p_python_functions->jac_action.m_arg[2] = p_python_functions->inputs.data();
  p_python_functions->jac_action.m_arg[3] = NV_DATA_S(v);
  p_python_functions->jac_action.m_res[0] = NV_DATA_S(rr);
  p_python_functions->jac_action();

  // tmp has -∂F/∂y˙ v
  realtype *tmp = p_python_functions->get_tmp();
  p_python_functions->mass_action.m_arg[0] = NV_DATA_S(v);
  p_python_functions->mass_action.m_res[0] = tmp;
  p_python_functions->mass_action();

  // AXPY: y <- a*x + y
  // rr has ∂F/∂y v + cj ∂F/∂y˙ v
  const int ns = p_python_functions->number_of_states;
  casadi::casadi_axpy(ns, -cj, tmp, NV_DATA_S(rr));

  return 0;
}

// Arguments tt is the current value of the independent variable t.
//   cj is the scalar in the system Jacobian, proportional to the inverse of the
//   step
//     size (α in Eq. (2.6) ).
//   yy is the current value of the dependent variable vector, y(t).
//   yp is the current value of ˙y(t).
//   rr is the current value of the residual vector F(t, y, y˙).
//   Jac is the output (approximate) Jacobian matrix (of type SUNMatrix), J =
//     ∂F/∂y + cj ∂F/∂y˙.
//   user data is a pointer to user data, the same as the user data parameter
//   passed to
//     IDASetUserData.
//   tmp1
//   tmp2
//   tmp3 are pointers to memory allocated for variables of type N Vector which
//   can
//     be used by IDALsJacFn function as temporary storage or work space.
int jacobian_casadi(realtype tt, realtype cj, N_Vector yy, N_Vector yp,
                    N_Vector resvec, SUNMatrix JJ, void *user_data,
                    N_Vector tempv1, N_Vector tempv2, N_Vector tempv3)
{

  CasadiFunctions *p_python_functions =
      static_cast<CasadiFunctions *>(user_data);

  // create pointer to jac data, column pointers, and row values
  sunindextype *jac_colptrs;
  sunindextype *jac_rowvals;
  realtype *jac_data;
  if (p_python_functions->options.dense_jacobian)
  {
    jac_data = SUNDenseMatrix_Data(JJ);
  }
  else
  {
    jac_colptrs = SUNSparseMatrix_IndexPointers(JJ);
    jac_rowvals = SUNSparseMatrix_IndexValues(JJ);
    jac_data = SUNSparseMatrix_Data(JJ);
  }

  // args are t, y, cj, put result in jacobian data matrix
  p_python_functions->jac_times_cjmass.m_arg[0] = &tt;
  p_python_functions->jac_times_cjmass.m_arg[1] = NV_DATA_S(yy);
  p_python_functions->jac_times_cjmass.m_arg[2] =
      p_python_functions->inputs.data();
  p_python_functions->jac_times_cjmass.m_arg[3] = &cj;
  p_python_functions->jac_times_cjmass.m_res[0] = jac_data;
  p_python_functions->jac_times_cjmass();

  if (!p_python_functions->options.dense_jacobian)
  {
    // row vals and col ptrs
    const int n_row_vals = p_python_functions->jac_times_cjmass_rowvals.size();
    auto p_jac_times_cjmass_rowvals =
        p_python_functions->jac_times_cjmass_rowvals.data();

    // std::cout << "jac_data = [";
    // for (int i = 0; i < p_python_functions->number_of_nnz; i++) {
    //   std::cout << jac_data[i] << " ";
    // }
    // std::cout << "]" << std::endl;

    // just copy across row vals (do I need to do this every time?)
    // (or just in the setup?)
    for (int i = 0; i < n_row_vals; i++)
    {
      // std::cout << "check row vals " << jac_rowvals[i] << " " <<
      // p_jac_times_cjmass_rowvals[i] << std::endl;
      jac_rowvals[i] = p_jac_times_cjmass_rowvals[i];
    }

    const int n_col_ptrs = p_python_functions->jac_times_cjmass_colptrs.size();
    auto p_jac_times_cjmass_colptrs =
        p_python_functions->jac_times_cjmass_colptrs.data();

    // just copy across col ptrs (do I need to do this every time?)
    for (int i = 0; i < n_col_ptrs; i++)
    {
      // std::cout << "check col ptrs " << jac_colptrs[i] << " " <<
      // p_jac_times_cjmass_colptrs[i] << std::endl;
      jac_colptrs[i] = p_jac_times_cjmass_colptrs[i];
    }
  }

  return (0);
}

int events_casadi(realtype t, N_Vector yy, N_Vector yp, realtype *events_ptr,
                  void *user_data)
{
  CasadiFunctions *p_python_functions =
      static_cast<CasadiFunctions *>(user_data);

  // std::cout << "EVENTS" << std::endl;
  // std::cout << "t = " << t << " y = [";
  // for (int i = 0; i < p_python_functions->number_of_states; i++) {
  //   std::cout << NV_DATA_S(yy)[i] << " ";
  // }
  // std::cout << "] yp = [";
  // for (int i = 0; i < p_python_functions->number_of_states; i++) {
  //   std::cout << NV_DATA_S(yp)[i] << " ";
  // }
  // std::cout << "]" << std::endl;

  // args are t, y, put result in events_ptr
  p_python_functions->events.m_arg[0] = &t;
  p_python_functions->events.m_arg[1] = NV_DATA_S(yy);
  p_python_functions->events.m_arg[2] = p_python_functions->inputs.data();
  p_python_functions->events.m_res[0] = events_ptr;
  p_python_functions->events();

  // std::cout << "events = [";
  // for (int i = 0; i < p_python_functions->number_of_events; i++) {
  //   std::cout << events_ptr[i] << " ";
  // }
  // std::cout << "]" << std::endl;

  return (0);
}

// This function computes the sensitivity residual for all sensitivity
// equations. It must compute the vectors
// (∂F/∂y)s i (t)+(∂F/∂ ẏ) ṡ i (t)+(∂F/∂p i ) and store them in resvalS[i].
// Ns is the number of sensitivities.
// t is the current value of the independent variable.
// yy is the current value of the state vector, y(t).
// yp is the current value of ẏ(t).
// resval contains the current value F of the original DAE residual.
// yS contains the current values of the sensitivities s i .
// ypS contains the current values of the sensitivity derivatives ṡ i .
// resvalS contains the output sensitivity residual vectors.
// Memory allocation for resvalS is handled within idas.
// user data is a pointer to user data.
// tmp1, tmp2, tmp3 are N Vectors of length N which can be used as
// temporary storage.
//
// Return value An IDASensResFn should return 0 if successful,
// a positive value if a recoverable error
// occurred (in which case idas will attempt to correct),
// or a negative value if it failed unrecoverably (in which case the integration
// is halted and IDA SRES FAIL is returned)
//
int sensitivities_casadi(int Ns, realtype t, N_Vector yy, N_Vector yp,
                         N_Vector resval, N_Vector *yS, N_Vector *ypS,
                         N_Vector *resvalS, void *user_data, N_Vector tmp1,
                         N_Vector tmp2, N_Vector tmp3)
{

  CasadiFunctions *p_python_functions =
      static_cast<CasadiFunctions *>(user_data);

  const int np = p_python_functions->number_of_parameters;

  // std::cout << "SENS t = " << t << " y = [";
  // for (int i = 0; i < p_python_functions->number_of_states; i++) {
  //   std::cout << NV_DATA_S(yy)[i] << " ";
  // }
  // std::cout << "] yp = [";
  // for (int i = 0; i < p_python_functions->number_of_states; i++) {
  //   std::cout << NV_DATA_S(yp)[i] << " ";
  // }
  // std::cout << "] yS = [";
  // for (int i = 0; i < p_python_functions->number_of_states; i++) {
  //   std::cout << NV_DATA_S(yS[0])[i] << " ";
  // }
  // std::cout << "] ypS = [";
  // for (int i = 0; i < p_python_functions->number_of_states; i++) {
  //   std::cout << NV_DATA_S(ypS[0])[i] << " ";
  // }
  // std::cout << "]" << std::endl;

  // for (int i = 0; i < np; i++) {
  //   std::cout << "dF/dp before = [" << i << "] = [";
  //   for (int j = 0; j < p_python_functions->number_of_states; j++) {
  //     std::cout << NV_DATA_S(resvalS[i])[j] << " ";
  //   }
  //   std::cout << "]" << std::endl;
  // }

  // args are t, y put result in rr
  p_python_functions->sens.m_arg[0] = &t;
  p_python_functions->sens.m_arg[1] = NV_DATA_S(yy);
  p_python_functions->sens.m_arg[2] = p_python_functions->inputs.data();
  for (int i = 0; i < np; i++)
  {
    p_python_functions->sens.m_res[i] = NV_DATA_S(resvalS[i]);
  }
  // resvalsS now has (∂F/∂p i )
  p_python_functions->sens();

  for (int i = 0; i < np; i++)
  {
    // std::cout << "dF/dp = [" << i << "] = [";
    // for (int j = 0; j < p_python_functions->number_of_states; j++) {
    //   std::cout << NV_DATA_S(resvalS[i])[j] << " ";
    // }
    // std::cout << "]" << std::endl;

    // put (∂F/∂y)s i (t) in tmp
    realtype *tmp = p_python_functions->get_tmp();
    p_python_functions->jac_action.m_arg[0] = &t;
    p_python_functions->jac_action.m_arg[1] = NV_DATA_S(yy);
    p_python_functions->jac_action.m_arg[2] = p_python_functions->inputs.data();
    p_python_functions->jac_action.m_arg[3] = NV_DATA_S(yS[i]);
    p_python_functions->jac_action.m_res[0] = tmp;
    p_python_functions->jac_action();

    // std::cout << "jac_action = [" << i << "] = [";
    // for (int j = 0; j < p_python_functions->number_of_states; j++) {
    //   std::cout << tmp[j] << " ";
    // }
    // std::cout << "]" << std::endl;

    const int ns = p_python_functions->number_of_states;
    casadi::casadi_axpy(ns, 1., tmp, NV_DATA_S(resvalS[i]));

    // put -(∂F/∂ ẏ) ṡ i (t) in tmp2
    p_python_functions->mass_action.m_arg[0] = NV_DATA_S(ypS[i]);
    p_python_functions->mass_action.m_res[0] = tmp;
    p_python_functions->mass_action();

    // std::cout << "mass_Action = [" << i << "] = [";
    // for (int j = 0; j < p_python_functions->number_of_states; j++) {
    //   std::cout << tmp[j] << " ";
    // }
    // std::cout << "]" << std::endl;

    // (∂F/∂y)s i (t)+(∂F/∂ ẏ) ṡ i (t)+(∂F/∂p i )
    // AXPY: y <- a*x + y
    casadi::casadi_axpy(ns, -1., tmp, NV_DATA_S(resvalS[i]));

    // std::cout << "resvalS[" << i << "] = [";
    // for (int j = 0; j < p_python_functions->number_of_states; j++) {
    //   std::cout << NV_DATA_S(resvalS[i])[j] << " ";
    // }
    // std::cout << "]" << std::endl;
  }

  return 0;
}
