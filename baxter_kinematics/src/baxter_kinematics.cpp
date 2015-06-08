// baxter_kinematics implementation file; start w/ fwd kin

//"include" path--should just be <baxter_kinematics/baxter_kinematics.h>, at least for modules outside this package
#include <baxter_kinematics/baxter_kinematics.h>
#include <eigen3/Eigen/src/Geometry/Transform.h>
using namespace std;

//ALL FNCS BELOW ARE FOR RIGHT ARM; EMULATE FOR CORRESPONDING LEFT-ARM METHODS AND VARS
Eigen::Matrix4d compute_A_of_DH(int i, double q_abb) {
    Eigen::Matrix4d A;
    Eigen::Matrix3d R;
    Eigen::Vector3d p;
    double a = DH_a_params[i];
    double d = DH_d_params[i];
    double alpha = DH_alpha_params[i];
    double q = q_abb + DH_q_offsets[i];

    A = Eigen::Matrix4d::Identity();
    R = Eigen::Matrix3d::Identity();
    //ROS_INFO("compute_A_of_DH: a,d,alpha,q = %f, %f %f %f",a,d,alpha,q);

    double cq = cos(q);
    double sq = sin(q);
    double sa = sin(alpha);
    double ca = cos(alpha);
    R(0, 0) = cq;
    R(0, 1) = -sq*ca; //% - sin(q(i))*cos(alpha);
    R(0, 2) = sq*sa; //%sin(q(i))*sin(alpha);
    R(1, 0) = sq;
    R(1, 1) = cq*ca; //%cos(q(i))*cos(alpha);
    R(1, 2) = -cq*sa; //%	
    //%R(3,1)= 0; %already done by default
    R(2, 1) = sa;
    R(2, 2) = ca;
    p(0) = a * cq;
    p(1) = a * sq;
    p(2) = d;
    A.block<3, 3>(0, 0) = R;
    A.col(3).head(3) = p;
    return A;
}

// same as above, but w/ spherical-wrist approx, i.e. ignore offset DH_a5
Eigen::Matrix4d compute_A_of_DH_approx(int i, double q_abb) {
    Eigen::Matrix4d A;
    Eigen::Matrix3d R;
    Eigen::Vector3d p;
    double a = DH_a_params_approx[i];
    double d = DH_d_params[i];
    double alpha = DH_alpha_params[i];
    double q = q_abb + DH_q_offsets[i];

    A = Eigen::Matrix4d::Identity();
    R = Eigen::Matrix3d::Identity();
    //ROS_INFO("compute_A_of_DH: a,d,alpha,q = %f, %f %f %f",a,d,alpha,q);

    double cq = cos(q);
    double sq = sin(q);
    double sa = sin(alpha);
    double ca = cos(alpha);
    R(0, 0) = cq;
    R(0, 1) = -sq*ca; //% - sin(q(i))*cos(alpha);
    R(0, 2) = sq*sa; //%sin(q(i))*sin(alpha);
    R(1, 0) = sq;
    R(1, 1) = cq*ca; //%cos(q(i))*cos(alpha);
    R(1, 2) = -cq*sa; //%	
    //%R(3,1)= 0; %already done by default
    R(2, 1) = sa;
    R(2, 2) = ca;
    p(0) = a * cq;
    p(1) = a * sq;
    p(2) = d;
    A.block<3, 3>(0, 0) = R;
    A.col(3).head(3) = p;
    return A;
}

Baxter_fwd_solver::Baxter_fwd_solver() { //(const hand_s& hs, const atlas_frame& base_frame, double rot_ang) {
    //this is a bit of a misnomer.  The Baxter URDF frame "right_lower_forearm" rotates as a function of q_s0.
    //However, D-H wants to define a frame with z0 axis along the s0 joint axis;
    //Define a static transform from arm_mount frame to D-H 0-frame
    A_rarm_mount_to_r_lower_forearm = Eigen::Matrix4d::Identity();
    A_rarm_mount_to_r_lower_forearm(0,3) = rmount_to_r_lower_forearm_x;
    A_rarm_mount_to_r_lower_forearm(1,3) = rmount_to_r_lower_forearm_y;
    A_rarm_mount_to_r_lower_forearm(2,3) = rmount_to_r_lower_forearm_z;
    Affine_rarm_mount_to_r_lower_forearm = A_rarm_mount_to_r_lower_forearm; // affine version of above
    //ROS_INFO("fwd_solver constructor");
    //there is also a static transform between torso and right-arm mount
    //manually populate values for this transform...
    A_torso_to_rarm_mount = Eigen::Matrix4d::Identity();
    A_torso_to_rarm_mount(0,3) = torso_to_rmount_x;
    A_torso_to_rarm_mount(1,3) = torso_to_rmount_y;
    A_torso_to_rarm_mount(2,3) = torso_to_rmount_z;  
    A_torso_to_rarm_mount(0,0) = cos(theta_z_arm_mount);
    A_torso_to_rarm_mount(0,0) = cos(theta_z_arm_mount); 
    A_torso_to_rarm_mount(1,1) = cos(theta_z_arm_mount);
    A_torso_to_rarm_mount(0,1) = -sin(theta_z_arm_mount);  
    A_torso_to_rarm_mount(1,0) = -A_torso_to_rarm_mount(0,1); 
    Affine_torso_to_rarm_mount = A_torso_to_rarm_mount;
}

Eigen::Affine3d Baxter_fwd_solver::fwd_kin_solve(const Vectorq7x1& q_vec) {
    Eigen::Matrix4d M;
    M = fwd_kin_solve_(q_vec);
    Eigen::Affine3d A(M);
    return A;
}

Eigen::Affine3d Baxter_fwd_solver::fwd_kin_solve_wrt_torso(const Vectorq7x1& q_vec) {
    Eigen::Matrix4d M;
    M = fwd_kin_solve_(q_vec);
    M = A_torso_to_rarm_mount*M;
    Eigen::Affine3d A(M);
    return A;
}

Eigen::Affine3d Baxter_fwd_solver::fwd_kin_solve_approx(const Vectorq7x1& q_vec) {
    Eigen::Matrix4d M;
    M = fwd_kin_solve_approx_(q_vec);
    Eigen::Affine3d A(M);
    return A;
}

Eigen::Matrix4d Baxter_fwd_solver::get_wrist_frame() {
    return A_mat_products[5]; // frames 4 and 5 have coincident origins
}

Eigen::Matrix4d Baxter_fwd_solver::get_shoulder_frame() {
    return A_mat_products[0]; // frame 1 has coincident origin, since a2=d2=0
}

Eigen::Matrix4d Baxter_fwd_solver::get_elbow_frame() {
    return A_mat_products[3]; // frames 2 and 3 have coincident origins
}

Eigen::Matrix4d Baxter_fwd_solver::get_flange_frame() {
    return A_mat_products[6];
}


Eigen::Matrix4d Baxter_fwd_solver::get_shoulder_frame_approx() {
    return A_mat_products_approx[0]; // frame 1 has coincident origin, since a2=d2=0
}

Eigen::Matrix4d Baxter_fwd_solver::get_elbow_frame_approx() {
    return A_mat_products_approx[3]; // frames 2 and 3 have coincident origins
}

Eigen::Matrix4d Baxter_fwd_solver::get_wrist_frame_approx() {
    Eigen::Matrix4d A_wrist;
    A_wrist = A_mat_products_approx[5];
    //cout<<"A_wrist from get_wrist: "<<endl;
    //cout<<A_wrist<<endl;
    return A_wrist; // frames 4 and 5 have coincident origins
}

Eigen::Matrix4d Baxter_fwd_solver::get_flange_frame_approx() {
    return A_mat_products_approx[6];
}

//fwd kin from frame 1 to wrist pt
Eigen::Vector3d Baxter_fwd_solver::get_wrist_coords_wrt_frame1(const Vectorq7x1& q_vec) {
    Eigen::Matrix4d A_shoulder_to_wrist;
    fwd_kin_solve_(q_vec);
    A_shoulder_to_wrist = A_mats[1]*A_mats[2]*A_mats[3]*A_mats[4];
    Eigen::Vector3d w_wrt_1 = A_shoulder_to_wrist.block<3, 1>(0, 3);
    return w_wrt_1;
}
/* Wrist Jacobian:  somewhat odd; restricted to q_s1, q_humerus and q_elbow
 * return a 3x3 Jacobian relating dq to delta wrist point coords, w/rt q_s1, q_humerus and q_elbow*/
// wrist coords are expressed w/rt frame1
// if q_forearm is known, use it--else set it to 0 or other approx
Eigen::Matrix3d Baxter_fwd_solver::get_wrist_Jacobian_3x3(double q_s1, double q_humerus, double q_elbow, double q_forearm) {
    Vectorq7x1 q_vec;
    for (int i=0;i<7;i++) q_vec(i)=0.0;
    q_vec(1) = q_s1;
    q_vec(2) = q_humerus;
    q_vec(3) = q_elbow;
    q_vec(4) = q_forearm;
    
    Eigen::Matrix4d A_mats_3dof[5];
    Eigen::Matrix4d A_mat_products_3dof[5];

    //Eigen::Matrix4d A = Eigen::Matrix4d::Identity();
    Eigen::Matrix4d Ai;
    Eigen::Matrix3d R;
    //Eigen::Vector3d p,t1,t2;;
    Eigen::Matrix3d Jw1_trans;
    Eigen::Matrix3d Jw1_ang;
    Eigen::Matrix3d Origins;
    //Eigen::Matrix3d Rvecs;
    Eigen::Vector3d zvec,rvec,wvec,Oi;
    //populate 5 A matrices and their products; need 5 just to get to wrist point, but can assume q_forearm=0, q_wrist_bend=0
    // note--starting from S1 frame, skipping frame 0
    for (int i=0;i<5;i++) {
        A_mats_3dof[i] = compute_A_of_DH(i+1, q_vec(i+1));
    }
        
    A_mat_products_3dof[0] = A_mats_3dof[0];
    //cout<<"A_mat_products_3dof[0]"<<endl;
    //cout<<A_mat_products_3dof[0]<<endl;
    for (int i=1;i<5;i++) {
        A_mat_products_3dof[i] = A_mat_products_3dof[i-1]*A_mats_3dof[i];
    }
    wvec = A_mat_products_3dof[4].block<3, 1>(0, 3); //strip off wrist coords
    //cout<<"wvec w/rt frame1: "<<wvec.transpose()<<endl;
    
    //compute the angular Jacobian, using z-vecs from each frame; first frame is just [0;0;1]
    zvec<<0,0,1;
    Jw1_ang.block<3, 1>(0, 0) = zvec; // and populate J_ang with them; at present, this is not being returned
    Oi<<0,0,0;
    Origins.block<3, 1>(0, 0) = Oi;
    for (int i=1;i<3;i++) {
        zvec = A_mat_products_3dof[i-1].block<3, 1>(0, 2); //%strip off z axis of each previous frame; note subscript slip  
        Jw1_ang.block<3, 1>(0, i) = zvec; // and populate J_ang with them;
        Oi = A_mat_products_3dof[i-1].block<3, 1>(0, 3); //origin of i'th frame
        Origins.block<3, 1>(0, i) = Oi;
    }    
    //now, use the zvecs to help compute J_trans
    for (int i=0;i<3;i++) {
        zvec = Jw1_ang.block<3, 1>(0, i); //%recall z-vec of current axis     
        Oi =Origins.block<3, 1>(0, i); //origin of i'th frame
        rvec = wvec - Oi; //%vector from origin of i'th frame to wrist pt 
        //Rvecs.block<3, 1>(0, i) = rvec; //save these?
        //t1 = zvecs.block<3, 1>(0, i);
        //t2 = rvecs.block<3, 1>(0, i);
        Jw1_trans.block<3, 1>(0, i) = zvec.cross(rvec);  
        //cout<<"frame "<<i<<": zvec = "<<zvec.transpose()<<"; Oi = "<<Oi.transpose()<<endl;
    }     
    //cout<<"J_ang: "<<endl;
    //cout<<Jw1_ang<<endl;
    return Jw1_trans;
}

// confirmed this function is silly...
// can easily transform Affine frames or A4x4 frames w:  Affine_torso_to_rarm_mount.inverse()*pose_wrt_torso;
Eigen::Affine3d Baxter_fwd_solver::transform_affine_from_torso_frame_to_arm_mount_frame(Eigen::Affine3d pose_wrt_torso) {
    //convert desired_hand_pose into equiv w/rt right-arm mount frame:
    /*
    Eigen::Affine3d desired_pose_wrt_arm_mount,desired_pose_wrt_arm_mount2;
    Eigen::Matrix3d R_hand_des_wrt_torso = pose_wrt_torso.linear();
    Eigen::Vector3d O_hand_des_wrt_torso = pose_wrt_torso.translation();
    Eigen::Vector3d O_hand_des_wrt_arm_mount;
    Eigen::Vector3d O_arm_mount_wrt_torso = A_torso_to_rarm_mount.col(3).head(3);
    Eigen::Matrix3d R_arm_mount_wrt_torso = A_torso_to_rarm_mount.block<3, 3>(0, 0);
    
    desired_pose_wrt_arm_mount.linear() = R_arm_mount_wrt_torso.transpose()*R_hand_des_wrt_torso;
 
    O_hand_des_wrt_arm_mount = R_arm_mount_wrt_torso.transpose()* O_hand_des_wrt_torso 
            - R_arm_mount_wrt_torso.transpose()* O_arm_mount_wrt_torso; //desired hand origin w/rt arm_mount frame
           
    desired_pose_wrt_arm_mount.translation() = O_hand_des_wrt_arm_mount;  
    cout<<"input pose w/rt torso: R"<<endl;
    cout<<pose_wrt_torso.linear()<<endl;
    cout<<"origin of des frame w/rt torso: "<<pose_wrt_torso.translation().transpose()<<endl;
    
    cout<<"input pose w/rt arm-mount frame: R"<<endl;
    cout<<desired_pose_wrt_arm_mount.linear()<<endl;    
    cout<<"origin of des frame w/rt arm-mount frame: "<<desired_pose_wrt_arm_mount.translation().transpose()<<endl;

    // now, try easier approach:
    desired_pose_wrt_arm_mount2 = Affine_torso_to_rarm_mount.inverse()*pose_wrt_torso;
     cout<<"input pose w/rt arm-mount frame, method 2: R"<<endl;
    cout<<desired_pose_wrt_arm_mount2.linear()<<endl;    
    cout<<"origin of des frame w/rt arm-mount frame, method 2: "<<desired_pose_wrt_arm_mount2.translation().transpose()<<endl;   

    
    return desired_pose_wrt_arm_mount;
     * */
    return Affine_torso_to_rarm_mount.inverse()*pose_wrt_torso;
}

//return soln out to tool flange; would still need to account for tool transform for gripper

//TESTED RIGHT-ARM FWD KIN on 5/27; LOOKS GOOD RELATIVE TO TF (w/rt right_arm_mount frame)
Eigen::Matrix4d Baxter_fwd_solver::fwd_kin_solve_(const Vectorq7x1& q_vec) {
    Eigen::Matrix4d A = Eigen::Matrix4d::Identity();
    //%compute A matrix from frame i to frame i-1:
    Eigen::Matrix4d A_i_iminusi;
    Eigen::Matrix3d R;
    Eigen::Vector3d p;
    for (int i = 0; i < 7; i++) {
        //A_i_iminusi = compute_A_of_DH(DH_a_params[i],DH_d_params[i],DH_alpha_params[i], q_vec[i] + DH_q_offsets[i] );
        A_i_iminusi = compute_A_of_DH(i, q_vec[i]);
        A_mats[i] = A_i_iminusi;
        //std::cout << "A_mats[" << i << "]:" << std::endl;
        //std::cout << A_mats[i] << std::endl;
    }

    A_mat_products[0] = A_mats[0];
    //account for static offset from Baxter's right-arm mount frame
    A_mat_products[0] = A_rarm_mount_to_r_lower_forearm*A_mat_products[0];
    for (int i = 1; i < 7; i++) {
        A_mat_products[i] = A_mat_products[i - 1] * A_mats[i];
    }
    
    return A_mat_products[6]; //tool flange frame
}

//same as above, but with spherical wrist approximation
Eigen::Matrix4d Baxter_fwd_solver::fwd_kin_solve_approx_(const Vectorq7x1& q_vec) {
    Eigen::Matrix4d A = Eigen::Matrix4d::Identity();
    //%compute A matrix from frame i to frame i-1:
    Eigen::Matrix4d A_i_iminusi;
    Eigen::Matrix3d R;
    Eigen::Vector3d p;
    for (int i = 0; i < 7; i++) {
        //A_i_iminusi = compute_A_of_DH(DH_a_params[i],DH_d_params[i],DH_alpha_params[i], q_vec[i] + DH_q_offsets[i] );
        A_i_iminusi = compute_A_of_DH_approx(i, q_vec[i]);
        A_mats_approx[i] = A_i_iminusi;
        //std::cout << "A_mats_approx[" << i << "]:" << std::endl;
        //std::cout << A_mats_approx[i] << std::endl;
    }

    A_mat_products_approx[0] = A_mats_approx[0];

    //account for static offset from Baxter's right-arm mount frame
    A_mat_products_approx[0] = A_rarm_mount_to_r_lower_forearm*A_mat_products_approx[0];
    //cout<<"A_mat_products_approx[0]"<<endl;
    //cout<<A_mat_products_approx[0]<<endl;    
    for (int i = 1; i < 7; i++) {
        A_mat_products_approx[i] = A_mat_products_approx[i - 1] * A_mats_approx[i];
        //std::cout << "A_mat_products_approx[" << i << "]:" << std::endl;
        //std::cout << A_mat_products_approx[i] << std::endl;        
    }
    //cout<<"A_mat_products_approx[6]"<<endl;
    //cout<<A_mat_products_approx[6]<<endl;    
    
    return A_mat_products_approx[6]; //tool flange frame
}

//IK methods:
Baxter_IK_solver::Baxter_IK_solver() {
    //constructor: 
    //ROS_INFO("Baxter_IK_solver constructor");
    L_humerus = 0.37082; // diag distance from shoulder to elbow; //DH_d_params[2];
    double L3 = DH_d3;
    double A3 = DH_a3;
    //L_humerus = sqrt(A3 * A3 + L3 * L3);
    L_forearm = DH_d5; // d-value is approx len, ignoring offset; 0.37442; // diag dist from elbow to wrist; //sqrt(A3 * A3 + L3 * L3);
    
    phi_shoulder= acos((-A3*A3+L_humerus*L_humerus+L3*L3)/(2.0*L3*L_humerus));
    // the following is redundant w/ fwd_solver instantiation, but repeat here, in case fwd solver
    // is not created
    //A_rarm_mount_to_r_lower_forearm = Eigen::Matrix4d::Identity();
    //A_rarm_mount_to_r_lower_forearm(0,3) = rmount_to_r_lower_forearm_x;
    //A_rarm_mount_to_r_lower_forearm(1,3) = rmount_to_r_lower_forearm_y;
    //A_rarm_mount_to_r_lower_forearm(2,3) = rmount_to_r_lower_forearm_z;  
    //cout<<"A_rarm_mount_to_r_lower_forearm"<<endl; // this was populated by inherited fwd_kin constructor
    //cout<<A_rarm_mount_to_r_lower_forearm<<endl;
}

//accessor function to get all solutions

void Baxter_IK_solver::get_solns(std::vector<Vectorq7x1> &q_solns) {
    q_solns = q_solns_fit; //q7dof_solns;
}

Eigen::Vector3d Baxter_IK_solver::wrist_frame0_from_tool_wrt_rarm_mount(Eigen::Affine3d affine_flange_frame) {
    Eigen::Vector3d flange_z_axis_wrt_arm_mount;
    Eigen::Vector3d flange_origin_wrt_arm_mount;
    Eigen::Vector3d wrist_pt_vec_wrt_arm_mount;
    Eigen::Matrix3d R_flange_wrt_arm_mount;
    
    R_flange_wrt_arm_mount = affine_flange_frame.linear();
    flange_origin_wrt_arm_mount = affine_flange_frame.translation(); 
    flange_z_axis_wrt_arm_mount = R_flange_wrt_arm_mount.col(2); 
    wrist_pt_vec_wrt_arm_mount = flange_origin_wrt_arm_mount-flange_z_axis_wrt_arm_mount*DH_d7;
        
    // this much looks correct...deduce wrist point from flange frame, w/rt arm mount frame
    //cout<<"wrist pt w/rt arm mount from flange pose and IK: "<<wrist_pt_vec_wrt_arm_mount.transpose()<<endl;
    return wrist_pt_vec_wrt_arm_mount;
}

//this is a misnomer:  should be wrist_pt_wrt_frame1_of_flange_des_and_qs0()
// given a desired hand pose, compute the wrist point by backing up from z_des by dist from flange to wrist;
// THEN, re-express this wrist point in the S1 coordinate frame--which requires knowledge of q_s0
Eigen::Vector3d Baxter_IK_solver::wrist_frame1_from_tool_wrt_rarm_mount(Eigen::Affine3d affine_flange_frame, Vectorq7x1 q_vec) {
    return wrist_pt_wrt_frame1_of_flange_des_and_qs0(affine_flange_frame,q_vec);
}


//this fnc assumes that affine_flange_frame is expressed w/rt arm-mount frame
Eigen::Vector3d Baxter_IK_solver::wrist_pt_wrt_frame1_of_flange_des_and_qs0(Eigen::Affine3d affine_flange_frame, Vectorq7x1 q_vec) {
    Eigen::Vector3d flange_z_axis_wrt_arm_mount;
    Eigen::Vector3d flange_origin_wrt_arm_mount;
    Eigen::Vector3d wrist_pt_wrt_arm_frame1;
    Eigen::Vector3d wrist_pt_vec_wrt_arm_mount;
    Eigen::Matrix3d R_flange_wrt_arm_mount;
    
    R_flange_wrt_arm_mount = affine_flange_frame.linear();
    flange_origin_wrt_arm_mount = affine_flange_frame.translation(); 
    flange_z_axis_wrt_arm_mount = R_flange_wrt_arm_mount.col(2); 
    wrist_pt_vec_wrt_arm_mount = flange_origin_wrt_arm_mount-flange_z_axis_wrt_arm_mount*DH_d7;
        
    // this much looks correct...deduce wrist point from flange frame, w/rt arm mount frame
    //cout<<"wrist pt w/rt arm mount from flange pose and IK: "<<wrist_pt_vec_wrt_arm_mount.transpose()<<endl;
    
    /////////////////alt--test vs approx fwd_kin: if wrist has no offset, can we find exact elbow angle?

    // may use dummy vals for q except q0:  
    //for (int i=1;i<7;i++) q_vec(i)=0.0;
    // run fwd kin, just to get A10 matrix--somewhat wasteful
    //Eigen::Affine3d A_fwd_DH = fwd_kin_solve(q_vec); //only really care about value of q_vec(0) for now
    //Eigen::Matrix4d A_wrist = get_wrist_frame();
    //std::cout << "fwd kin2 wrist point: " << A_wrist(0, 3) << ", " << A_wrist(1, 3) << ", " << A_wrist(2, 3) << std::endl;
     
    //next line is somewhat wasteful; computes fwd kin just to get A_shoulder_wrt_arm_mount
    //although all 7 frames are computed, only the first is used/needed;
    //only requires specification of q0;  q1 through q6 can be arbitrary
    Eigen::Affine3d A_fwd_DH_approx = fwd_kin_solve_approx(q_vec);
    
    //cout<<"A_fwd_DH_approx.linear(): "<<endl;
    //cout<<A_fwd_DH_approx.linear()<<endl;
    //cout<<"trans: "<<A_fwd_DH_approx.translation().transpose()<<endl;
    //Eigen::Matrix4d A_wrist_approx = get_wrist_frame_approx();
    //cout<<"A_wrist_approx:"<<endl;
    //cout<<A_wrist_approx<<endl;
    //std::cout << "approx FK wrist point: " << A_wrist_approx(0, 3) << ", " << A_wrist_approx(1, 3) << ", " << A_wrist_approx(2, 3) << std::endl;    
    
    // OVERWRITE WRIST POINT USING APPROX FK--FOR TEST/VALIDATION ONLY
    // DELETE THIS AFTER TESTING
    // will NOT have q_vec available for fwd_kin;
    // must use wrist pt derived from flange frame and IK
    //for (int i=0;i<3;i++) {
    //    wrist_pt_vec_wrt_arm_mount(i) = A_wrist_approx(i,3);
    //}
    
    //cout<<"approx wrist pt w/rt arm mount from approx FK: "<<wrist_pt_vec_wrt_arm_mount.transpose()<<endl;
    
    //test--make sure frames are the same as fwd_kin:
                //get and print solutions--origins off selected frames

    //Eigen::Matrix4d A_elbow = get_elbow_frame();
    //std::cout << "fwd kin2 elbow point: " << A_elbow(0, 3) << ", " << A_elbow(1, 3) << ", " << A_elbow(2, 3) << std::endl;
 
            
    Eigen::Matrix4d A_shoulder_wrt_arm_mount = get_shoulder_frame_approx();
    //std::cout << "fwd kin2 shoulder point: " << A_shoulder_wrt_arm_mount(0, 3) << ", " << A_shoulder_wrt_arm_mount(1, 3) << ", " << A_shoulder_wrt_arm_mount(2, 3) << std::endl;

    Eigen::Matrix3d R_shoulder_wrt_arm_mount = A_shoulder_wrt_arm_mount.block<3, 3>(0, 0);
    //cout<<"R_shoulder_wrt_arm_mount:"<<endl;
    //cout<<R_shoulder_wrt_arm_mount<<endl;
    Eigen::Vector3d p1_wrt_0 = A_shoulder_wrt_arm_mount.col(3).head(3); ///origin of frame1 w/rt frame0  
    
    //cout<<"p1_wrt_0: "<<p1_wrt_0.transpose()<<endl;
    /*
        R10 = A10.block<3, 3>(0, 0);
    p1_wrt_0 = A10.col(3).head(3); //origin of frame1 w/rt frame0; should be the same as above
    //compute A10_inv * w_wrt_0, = R'*w_wrt_0 -R'*p1_wrt_0
    w_wrt_1b = R10.transpose() * w_des - R10.transpose() * p1_wrt_0; //desired wrist pos w/rt frame1
    */
   //compute A10_inv * w_wrt_0, = R'*w_wrt_0 -R'*p1_wrt_0
    wrist_pt_wrt_arm_frame1 = R_shoulder_wrt_arm_mount.transpose() * wrist_pt_vec_wrt_arm_mount 
            - R_shoulder_wrt_arm_mount.transpose() * p1_wrt_0; //desired wrist pos w/rt frame1 
    //cout<<"wrist_pt_wrt_arm_frame1: "<<wrist_pt_wrt_arm_frame1.transpose()<<endl;
    return wrist_pt_wrt_arm_frame1;
    
}

// in this version, compute the wrist point from the desired flange frame--independent of what
// reference frame is used for the flange frame.  Wrist coords will be in the same reference frame
Eigen::Vector3d Baxter_IK_solver::wrist_pt_from_flange_frame(Eigen::Affine3d affine_flange_frame) {
    Eigen::Vector3d flange_z_axis;
    Eigen::Vector3d flange_origin;
    Eigen::Vector3d wrist_pt;
    Eigen::Matrix3d R_flange=affine_flange_frame.linear();
    
    flange_origin = affine_flange_frame.translation(); 
    flange_z_axis = R_flange.col(2); 
    wrist_pt = flange_origin-flange_z_axis*DH_d7;
    return wrist_pt;
}

bool Baxter_IK_solver::fit_q_to_range(double q_min, double q_max, double &q) {
    while (q<q_min) {
        q+= 2.0*M_PI;
    }
    while (q>q_max) {
        q-= 2.0*M_PI;
    }    
    if (q<q_min)
        return false;
    else
        return true;
}

bool Baxter_IK_solver::fit_joints_to_range(Vectorq7x1 &qvec) {
    bool fits=true;
    bool does_fit;
    double q;
    for (int i=0;i<7;i++) {
        q = qvec[i];
        does_fit = fit_q_to_range(q_lower_limits[i],q_upper_limits[i],q);
        qvec[i] = q;
        fits = fits&&does_fit;
    }
    if (fits)
        return true;
    else
        return false;
}

int Baxter_IK_solver::ik_solve(Eigen::Affine3d const& desired_hand_pose) // solve IK
{ return 0; // dummy
}



int Baxter_IK_solver::ik_solve_approx_wrt_torso(Eigen::Affine3d const& desired_hand_pose_wrt_torso,std::vector<Vectorq7x1> &q_solns) {
    //convert desired_hand_pose into equiv w/rt right-arm mount frame:
    //Eigen::Affine3d desired_hand_pose_wrt_arm_mount = transform_affine_from_torso_frame_to_arm_mount_frame(desired_hand_pose);
    Eigen::Affine3d desired_hand_pose_wrt_arm_mount = Affine_torso_to_rarm_mount.inverse()*desired_hand_pose_wrt_torso;
    ik_solve_approx(desired_hand_pose_wrt_arm_mount,q_solns);
}

// in this version, soln ONLY for specified q_s0;  specify q_s0 and desired hand pose, w/rt torso
// expect from 0 to 4 solutions at given q_s0
int Baxter_IK_solver::ik_solve_approx_wrt_torso_given_qs0(Eigen::Affine3d const& desired_hand_pose_wrt_torso,double q_s0, std::vector<Vectorq7x1> &q_solns) {
    //convert desired_hand_pose into equiv w/rt right-arm mount frame:
    //Eigen::Affine3d desired_hand_pose_wrt_arm_mount = transform_affine_from_torso_frame_to_arm_mount_frame(desired_hand_pose);
    Eigen::Affine3d desired_hand_pose_wrt_arm_mount = Affine_torso_to_rarm_mount.inverse()*desired_hand_pose_wrt_torso;
    Eigen::Matrix3d Rdes = desired_hand_pose_wrt_arm_mount.linear();
    q_solns.clear();

    std::vector<Vectorq7x1> q_solns_of_qs0, q_solns_w_wrist;

    Vectorq7x1 q_soln;
    
    int nsolns=0;  
    bool reachable;
    reachable = compute_q123_solns(desired_hand_pose_wrt_arm_mount, q_s0, q_solns_of_qs0);    
    cout<<"ik_solve_approx_wrt_torso_given_qs0 num solns: "<<q_solns_of_qs0.size()<<endl;
 
    //now, compute corresponding wrist solns and add successful results to list of 7dof solns:   
    for (int i=0; i< q_solns_of_qs0.size(); i++)
    {
        q_soln = q_solns_of_qs0[i];
        cout<<"q_soln123: "<<q_soln.transpose()<<endl;
        solve_spherical_wrist(q_soln,Rdes, q_solns_w_wrist); 
        for (int j=0;j<q_solns_w_wrist.size();j++) {
            q_solns.push_back(q_solns_w_wrist[j]); // push these on in order, from q_s0_ctr towards q_s0_min
            cout<<"q_solnw: "<<q_solns_w_wrist[j].transpose()<<endl;
        }
    }          
    cout<<"there are "<<q_solns.size()<<" solutions with wrist options"<<endl;
    return q_solns.size(); // return number of solutions found            
}



// assumes desired_hand_pose is w/rt D-H frame 0;
// use ik_solve_approx_wrt_torso if hand frame is expressed w/rt torso
//major fnc: samples values of q_s0 at resolution DQS0 to compute a vector of viable, approximate solutions to IK of desired_hand_pose
// for solutions of interest, can subsequently refine the precision of these with precise_soln_q123, etc.
int Baxter_IK_solver::ik_solve_approx(Eigen::Affine3d const& desired_hand_pose,std::vector<Vectorq7x1> &q_solns) // given desired hand pose, find all viable IK solns
{ 
  double q_s0_ctr = compute_qs0_ctr(desired_hand_pose);
  double dqs0 = DQS0; // search resolution on dq0
  double  q_s0 = q_s0_ctr;
  std::vector<Vectorq7x1> q_solns_123, q_solns_of_qs0, q_solns_w_wrist;
  Vectorq7x1 q_soln;
  int nsolns=0;
  q_solns.clear(); // fill in all valid solns in this list
  q_solns_123.clear();  
  Eigen::Vector3d w_des_wrt_0 = wrist_frame0_from_tool_wrt_rarm_mount(desired_hand_pose);
  Eigen::Affine3d A_fwd_DH;  
  Eigen::Matrix4d A_wrist;
  Eigen::Matrix3d Rdes = desired_hand_pose.linear();
  bool reachable = true;  
  while (reachable) { 
        cout<<"try q_s0 = "<<q_s0<<endl;
        reachable = compute_q123_solns(desired_hand_pose, q_s0, q_solns_of_qs0); 
        if (reachable ) {
            //test these solns:
            cout<<"test: des w_wrt_0 = "<<w_des_wrt_0.transpose()<<endl;
            for (int i=0;i<q_solns_of_qs0.size();i++) {
                A_fwd_DH = fwd_kin_solve(q_solns_of_qs0[i]);
                A_wrist = get_wrist_frame();
                std::cout << "sln"<<i<<":  w_wrt_0 " << A_wrist(0, 3) << ", " << A_wrist(1, 3) << ", " << A_wrist(2, 3) << std::endl;
                q_solns_123.push_back(q_solns_of_qs0[i]);
            }
            q_s0+= dqs0;
        }
    }
    cout<<"found q_s0 max = "<<q_s0<<endl;
    reachable = true;
    //now, order these in reverse, and tack on wrist solns
    for (int i=q_solns_123.size()-1; i>=0;i--)
    {
        q_soln = q_solns_123[i];
        solve_spherical_wrist(q_soln,Rdes, q_solns_w_wrist); 
        for (int j=0;j<q_solns_w_wrist.size();j++) {
            q_solns.push_back(q_solns_w_wrist[j]); // push these on in reverse order, from q_s0_max towards q_s0_ctr
        }
    }
    cout<<"pushed "<<q_solns.size()<<" solns in fwd search qs0"<<endl;
    // now search in neg rot of q_s0 from center:
    q_s0 = q_s0_ctr - dqs0;
    q_solns_123.clear();
    while (reachable) { 
        cout<<"try q_s0 = "<<q_s0<<endl;
        reachable = compute_q123_solns(desired_hand_pose, q_s0, q_solns_of_qs0); 
        if (reachable ) {
            //test these solns:
            cout<<"test: des w_wrt_0 = "<<w_des_wrt_0.transpose()<<endl;
            for (int i=0;i<q_solns_of_qs0.size();i++) {
                A_fwd_DH = fwd_kin_solve(q_solns_of_qs0[i]);
                A_wrist = get_wrist_frame();
                std::cout << "sln"<<i<<":  w_wrt_0 " << A_wrist(0, 3) << ", " << A_wrist(1, 3) << ", " << A_wrist(2, 3) << std::endl;
                q_solns_123.push_back(q_solns_of_qs0[i]);
            }
            q_s0-= dqs0;
        }
    }
    cout<<"found q_s0 min = "<<q_s0<<endl;
    reachable = true;
    //now, tack on wrist solns and add to list of solns:
    
    for (int i=0; i< q_solns_123.size(); i++)
    {
        q_soln = q_solns_123[i];
        solve_spherical_wrist(q_soln,Rdes, q_solns_w_wrist); 
        for (int j=0;j<q_solns_w_wrist.size();j++) {
            q_solns.push_back(q_solns_w_wrist[j]); // push these on in order, from q_s0_ctr towards q_s0_min
        }
    }       
    cout<<"pushed "<<q_solns_123.size()<<" solns in rvrs search qs0"<<endl;      
return q_solns.size(); // return number of solutions found
}

//this function takes a desired_hand_pose (with respect to torso frame) and computes elbow options, organized as:
//samples of q_s0 from q_s0_min to q_s0_max define "layers"
// each "layer" contains IK solns at fixed q_s0

int Baxter_IK_solver::ik_solve_approx_elbow_orbit_from_flange_pose_wrt_torso(Eigen::Affine3d const& desired_hand_pose_wrt_torso,std::vector<std::vector<Eigen::VectorXd> > &path_options) {
    //std::vector<std::vector<Eigen::VectorXd> > path_options; 
  Eigen::Affine3d   desired_hand_pose_wrt_rarm_mount=Affine_torso_to_rarm_mount.inverse()*desired_hand_pose_wrt_torso; //related to torso via static transforms 
  Eigen::Matrix3d   Rdes = desired_hand_pose_wrt_rarm_mount.linear();
  double q_s0_ctr = compute_qs0_ctr(desired_hand_pose_wrt_rarm_mount);
  double dqs0 = DQS0; // search resolution on dq0
  double q_s0= q_s0_ctr;

    std::vector<Eigen::VectorXd>  single_layer_nodes; 
    std::vector<std::vector<Eigen::VectorXd> > path_options_reverse;   
    Eigen::VectorXd  node; 
    int nsolns;
    std::vector<Vectorq7x1> q_solns_of_qs0; 
    path_options.clear();
    
   bool reachable = true; 
   // find solns in order from q_s0 to q_s0_max; reorder these solns later
  while (reachable) { 
        cout<<"try q_s0 = "<<q_s0<<endl;
        nsolns =ik_solve_approx_wrt_torso_given_qs0(desired_hand_pose_wrt_torso,q_s0, q_solns_of_qs0);
        if (nsolns==0) reachable=false;
        if (reachable ) {
                 single_layer_nodes.clear();            
            for (int i=0;i<nsolns;i++) {
                // this is annoying: can't treat std::vector<Vectorq7x1> same as std::vector<Eigen::VectorXd> 
                node = q_solns_of_qs0[i];
                single_layer_nodes.push_back(node);
            }
            path_options_reverse.push_back(single_layer_nodes);     
            q_s0+= dqs0;
        }
    }
    cout<<"found q_s0 max = "<<q_s0<<endl;   
    
    //now, order these in reverse
    for (int i=path_options_reverse.size()-1; i>=0;i--)
    {
        path_options.push_back(path_options_reverse[i]);
    }
    cout<<"pushed "<<path_options_reverse.size()<<" solns in fwd search qs0"<<endl;
    
      // now search in neg rot of q_s0 from center:
    reachable = true; 

    q_s0 = q_s0_ctr - dqs0;
    while (reachable) { 
        cout<<"try q_s0 = "<<q_s0<<endl;
        nsolns =ik_solve_approx_wrt_torso_given_qs0(desired_hand_pose_wrt_torso,q_s0, q_solns_of_qs0);
        if (nsolns==0) reachable=false;
        if (reachable ) {
            single_layer_nodes.clear();            
            for (int i=0;i<nsolns;i++) {
                node = q_solns_of_qs0[i];
                single_layer_nodes.push_back(node);
            }
            path_options.push_back(single_layer_nodes);     
            q_s0-= dqs0;
        }
    }
    cout<<"found q_s0 min = "<<q_s0<<endl;
   
    //desired_hand_pose_wrt_torso
    return path_options.size(); // should be number of q_s0 samples
}

//function to find precise values of joint angles q1, q2, q3 to match desired wrist position, implied by desired_hand_pose
//provide q123_approx; this function will take q_s0 and q_forearm as specified, and q_s1, q_humerus and q_elbow as approximated,
// and will refine q_s1, q_humerus and q_elbow to attempt a precise fit to desired wrist position;
// improved soln is returned in q123_precise
// desired_hand_pose must be expressed w/rt right-arm mount frame (not torso frame)
double  Baxter_IK_solver::precise_soln_q123(Eigen::Affine3d const& desired_hand_pose,Vectorq7x1 q123_approx, Vectorq7x1 &q123_precise) {
        double q_s0 = q123_approx[0];

        Eigen::Vector3d w_approx,w_err,dq123,wrist_pt_wrt_right_arm_frame1;
        Eigen::Matrix3d Jw3x3,Jw3x3_inv;
        //compute the desired wrist point, working backwards from desired hand frame
        //provide q_s0 in q123_approx, so this can be expressed w/rt frame1
        wrist_pt_wrt_right_arm_frame1= wrist_pt_wrt_frame1_of_flange_des_and_qs0(desired_hand_pose,q123_approx);
        Jw3x3 = get_wrist_Jacobian_3x3(q123_approx[1], q123_approx[2], q123_approx[3], q123_approx[4]);          
        //cout<<"Jw3x3: "<<endl;
        //cout<<Jw3x3<<endl;
        //cout<<"det = "<<Jw3x3.determinant()<<endl;
        Jw3x3_inv =  Jw3x3.inverse();
        //cout<<"Jw3x3_inv: "<<endl;
        //cout<<Jw3x3_inv<<endl;        
         q123_precise=  q123_approx; // start w/ given seed guess
        int jiter=0;
        double w_err_norm = 1.0;
        while ((jiter<MAX_JINV_ITERS)&&(w_err_norm>W_ERR_TOL))
        {
            // iterate w/ Jacobian to find more precise soln
            w_approx = get_wrist_coords_wrt_frame1(q123_precise);
            w_err = wrist_pt_wrt_right_arm_frame1-w_approx;
            w_err_norm = w_err.norm();
            cout<<"iter "<<jiter<<"; w_err_norm = "<<w_err_norm<< "; w_err =  "<<w_err.transpose()<<endl;
            dq123 = Jw3x3_inv*w_err;
            if (dq123.norm()> DQ_ITER_MAX) {
                dq123/=DQ_ITER_MAX; //protect against numerical instability
            }
            cout<<"dq123: "<<dq123.transpose()<<endl;
            for (int i=1;i<4;i++) {
                q123_precise[i]+=dq123[i-1];
            }
            jiter++;
        }
        return w_err_norm;
}

// assumes desired_hand_pose is expressed w/rt r_arm_mount
double  Baxter_IK_solver::compute_qs0_ctr(Eigen::Affine3d const& desired_hand_pose)
{ 
    bool reachable = false;
    Eigen::Vector3d w_wrt_1; 
    Vectorq7x1 soln1_vec;
    soln1_vec(0)=0.0;
    w_wrt_1 = wrist_frame1_from_tool_wrt_rarm_mount(desired_hand_pose,soln1_vec);
    cout<<"compute_qs0_ctr: wrist pt wrt frame 1 at q_s0=0 "<<w_wrt_1.transpose()<<endl;    
    double r_perp = sqrt(w_wrt_1(1)*w_wrt_1(1)+w_wrt_1(0)*w_wrt_1(0));
    double q_s0_ctr = atan2(w_wrt_1(2),r_perp);
    //TEST:
    cout<<"q_s0_ctr="<<q_s0_ctr<<endl;
    soln1_vec(0)=q_s0_ctr;
    
    //reachable = compute_q123_solns(desired_hand_pose, q_s0, q_solns);    
    //recompute w_wrt_1 using q_s0_ctr--intend for w_z = 0 at this pose
    w_wrt_1 = wrist_frame1_from_tool_wrt_rarm_mount(desired_hand_pose,soln1_vec);   
    cout<<"compute_qs0_ctr: wrist pt wrt frame 1 at q_s0_ctr: "<<w_wrt_1.transpose()<<endl;
    
    return q_s0_ctr;
}

//this fnc assumes that affine_flange_frame is expressed w/rt arm-mount frame
// put together [q_s1,q_humerus,q_elbow] solns as fnc (q_s0)
// there will be 0, 1 or 2 solutions;
// pack these into a 7x1 vector--just leave wrist DOFs =0 for now;
// return "true" if at least 1 valid soln within joint ranges
bool Baxter_IK_solver::compute_q123_solns(Eigen::Affine3d const& desired_hand_pose, double q_s0, std::vector<Vectorq7x1> &q_solns)
{ 
    q_solns.clear();
    Vectorq7x1 soln1_vec;
    Vectorq7x1 soln2_vec;  
    //clear out solns:
    for (int i=0;i<7;i++) {
        soln1_vec[i] = 0.0; 
        soln2_vec[i] = 0.0;        
    }
    soln1_vec[0] = q_s0; // this is given
    soln2_vec[0] = q_s0;
    double q_elbow;
    double q_humerus[2];
    double q_s1[2];
    double q_s1_temp;
    bool does_fit;
    bool reachable=false;
    bool at_least_one_valid_soln = false;
    Eigen::Vector3d wrist_pt_wrt_right_arm_frame1; 
    
    //given desired hand pose, compute the approximate wrist point coordinates, and express these w/rt frame 1 (s1 frame)
    wrist_pt_wrt_right_arm_frame1 = wrist_frame1_from_tool_wrt_rarm_mount(desired_hand_pose,soln1_vec);
    //cout<< "compute_q123_solns: w w/rt frame1 from fwd kin code: "<<wrist_pt_wrt_right_arm_frame1.transpose()<<endl;    
    
    //solve for q_elbow;
    reachable= solve_for_elbow_ang(wrist_pt_wrt_right_arm_frame1, q_elbow);  
    if (!reachable) {
        ROS_WARN("compute_q123_solns: solve_for_elbow_ang returned false");
        return false; // give up--point is out of reach--regardless of joint limits
    }
    
    does_fit = fit_q_to_range(q_lower_limits[3],q_upper_limits[3],q_elbow); // confirm q_elbow is within joint limits
    if (!does_fit) {
        cout<< "compute_q123_solns: elbow soln out of range; quitting"<<endl;
        return false; //give up--elbow angle solution is unreachable
    } 
    
    // OK--q_elbow is within joint-limit range; let's save it:
    soln1_vec[3] = q_elbow; 
    soln2_vec[3] = q_elbow; 
    
    // next, see if we can get humerus solutions:
    reachable = solve_for_humerus_ang(wrist_pt_wrt_right_arm_frame1,q_elbow, q_humerus); //expect 2 q_humerus values
    if (!reachable) {
        cout<<"compute_q123_solns: not reachable w/ humerus soln; quitting"<<endl;
        return false; // give up--point is out of reach w/rt q_humerus--regardless of joint limits
    }
    soln1_vec[2] = q_humerus[0]; // save these 2 potential solns
    soln2_vec[2] = q_humerus[1]; 
    //double q_h = q_humerus[0];
    // so far, so good; process q_humerus[0] first
    does_fit = fit_q_to_range(q_lower_limits[2],q_upper_limits[2],q_humerus[0]); // confirm q_humerus is within joint limits
    if (does_fit)  {
        //if here, continue to process q_humerus[0]; find corresponding q_s1
        //cout<<"q_humerus[0] is in range: "<<q_humerus[0]<<endl;
        //solve for q_shoulder_elevation
        reachable = solve_for_s1_ang(wrist_pt_wrt_right_arm_frame1,q_elbow,  q_humerus[0], q_s1_temp);
        cout<<"testing q_s1 = "<<q_s1_temp<<endl;
        if (reachable) { //got a soln for q_s1...but is it in joint range?
            if (fit_q_to_range(q_lower_limits[1],q_upper_limits[1],q_s1_temp)) {
                //if here, then we have a complete, legal soln; push it onto the soln vector
                soln1_vec[1] = q_s1_temp;
                q_solns.push_back(soln1_vec);
                at_least_one_valid_soln = true;
                cout<<"soln vec: "<<soln1_vec.transpose()<<endl;
            }      

        else {  cout<<"qs1 soln not in joint range"<<endl;
                } 
        }
    }
    else {
        cout<<"q_humerus[0] is out of range: "<<q_humerus[0]<<endl;
    }
    // repeat for 2nd q_humerus soln:
    does_fit = fit_q_to_range(q_lower_limits[2],q_upper_limits[2],q_humerus[1]); // confirm q_humerus is within joint limits
    if (does_fit)  {
        //if here, continue to process q_humerus[0]; find corresponding q_s1
        //cout<<"q_humerus[1] is in range: "<<q_humerus[1]<<endl;
        //solve for q_shoulder_elevation
        reachable = solve_for_s1_ang(wrist_pt_wrt_right_arm_frame1,q_elbow,  q_humerus[1], q_s1_temp);
        cout<<"testing q_s1 = "<<q_s1_temp<<endl;
        if (reachable) { //got a soln for q_s1...but is it in joint range?
            if (fit_q_to_range(q_lower_limits[1],q_upper_limits[1],q_s1_temp)) {
                //if here, then we have a complete, legal soln; push it onto the soln vector
                soln2_vec[1] = q_s1_temp;
                q_solns.push_back(soln2_vec);
                at_least_one_valid_soln = true;
                cout<<"soln vec: "<<soln2_vec.transpose()<<endl;
            }      
            else {
                cout<<"qs1 soln not in joint range"<<endl;
            }
        }        
    }    
    else  {
        cout<<"q_humerus[1] is out of range: "<<q_humerus[1]<<endl;  
    }
              
    return at_least_one_valid_soln; //give up--elbow angle solution is unreachable   
    
}


bool Baxter_IK_solver::solve_for_elbow_ang(Eigen::Vector3d w_wrt_1,double &q_elbow) {
    double r_goal = w_wrt_1.norm();
    //cout<<"r_goal: "<<r_goal<<endl;
    double acos_arg = (L_humerus*L_humerus + L_forearm*L_forearm - r_goal*r_goal)/(2.0*L_humerus*L_forearm);
    if ((fabs(acos_arg)>1.0)||(r_goal>r_goal_max)) {
        ROS_WARN("solve_for_elbow_ang beyond safe reach;  acos_arg = %f; r_goal = %f",acos_arg,r_goal);
        return false;
    }
    //cout<<"acos_arg: "<<acos_arg<<endl;
    double eta = acos(acos_arg); 
    //cout<<"eta: "<<eta<<endl;
    
    double q_elbow_nom = M_PI - eta; // 
    //cout<<"q_elbow_nom="<<q_elbow_nom<<endl;
    q_elbow = q_elbow_nom+ phi_shoulder;  //this should be the viable solution
    //q_elbow[1] = -q_elbow_nom + phi_shoulder;
    //cout<<"q_elbow = "<<q_elbow<<endl;
    /*
    ROS_INFO("test:");
    Eigen::Matrix4d    A_12 = compute_A_of_DH_approx(1, 0);    
    Eigen::Matrix4d    A_23 = compute_A_of_DH_approx(2, 0); 
    Eigen::Matrix4d    A_34 = compute_A_of_DH_approx(3, q_elbow);
    Eigen::Matrix4d    A_45 = compute_A_of_DH_approx(4, 0);    
    Eigen::Matrix4d    A_15 = A_12*A_23*A_34*A_45;
    cout<<"A_15(q_elbow) = "<<endl;
    cout<<A_15<<endl;
    Eigen::Vector3d p_fwd_wrt_1 = A_15.col(3).head(3); //origin of frame1 w/rt frame0; should be the same as above
    double r_soln = p_fwd_wrt_1.norm();
    cout<<"r_soln = "<<r_soln<<endl;
    */
    return true;
}

//given desired wrist point, w/rt frame 1, and given q_elbow, solve for q_humerus--2 solns
bool Baxter_IK_solver::solve_for_humerus_ang(Eigen::Vector3d w_wrt_1,double q_elbow, double q_humerus[2]) {
    //moment arm of humerus rotation is elbow offset plus displacement of wrist due to elbow rotation
    double r_arm = DH_a3 + sin(q_elbow)*L_forearm; // w/ bent elbow, will always be positive
    double z_offset = w_wrt_1(2);

    //ROS_INFO("z_offset des = %f; r_arm = %f",z_offset,r_arm);
    if (fabs(z_offset)>r_arm) {
        ROS_WARN("no humerus solution: |z_offset| > r_arm ");
        return false;
    }
    q_humerus[0] = asin(z_offset/r_arm);
    q_humerus[1] = M_PI-q_humerus[0];
    //ROS_INFO("q_humerus = %f, %f",q_humerus[0],q_humerus[1]);
    /*
    ROS_INFO("test:");
    Eigen::Matrix4d    A_12 = compute_A_of_DH_approx(1, 0);    
    Eigen::Matrix4d    A_23 = compute_A_of_DH_approx(2, q_humerus[0]); 
    Eigen::Matrix4d    A_34 = compute_A_of_DH_approx(3, q_elbow);
    Eigen::Matrix4d    A_45 = compute_A_of_DH_approx(4, 0);    
    Eigen::Matrix4d    A_15 = A_12*A_23*A_34*A_45;
    cout<<"A_15(q_humerus[0],q_elbow) = "<<endl;
    cout<<A_15<<endl;    
    A_23 = compute_A_of_DH_approx(2, q_humerus[1]); 
    A_15 = A_12*A_23*A_34*A_45;
    cout<<"A_15(q_humerus[1],q_elbow) = "<<endl;
    cout<<A_15<<endl;  
     * */
    return true;
}    
    

//given desired wrist point, w/rt frame 1, and given q_elbow, and q_humerus, solve for shoulder-elevation angle;
// need to call this method twice--for 2 different solns for q_humerus
bool Baxter_IK_solver::solve_for_s1_ang(Eigen::Vector3d w_wrt_1,double q_elbow, double q_humerus, double &q_s1) {
   //moment arm of humerus rotation is elbow offset plus displacement of wrist due to elbow rotation
    double r_reach = w_wrt_1.norm(); //dist from frame-1 origin to wrist point
    // perpendicular radius from z3 to w is r_perp
    double r_perp = DH_a3 + sin(q_elbow)*L_forearm; // w/ bent elbow, will always be positive
    double Lw_sqd = r_reach*r_reach - r_perp*r_perp; //compute distance along z3 from O1 to w

    if (Lw_sqd<0)
    {
        ROS_WARN("solve_for_s1_ang: Lw negative! no soln");
        return false;
    }
    double Lw = sqrt(Lw_sqd);
    //ROS_INFO("r_perp, Lw = %f, %f",r_perp,Lw);    
    // now, have eqn: 
    //r_perp*cos(q_s1)*cos(q_humerus) + Lw*sin(q_s1) = r_perp*cos(q_humerus)
    // or a*cos(q) + b*sin(q) = c
    // first soln:
 
    double b = Lw;
    double c = w_wrt_1(0);
    
    double a = r_perp*cos(q_humerus);    
    double alpha = atan2(b,a);
    double rtemp = sqrt(a*a+b*b);
    //now, have rtemp*cos(q-alpha) = c
    double cos_of_arg = c/rtemp;
    if (fabs(cos_of_arg)>1.0) {
        ROS_WARN("solve_for_s1_ang: logic problem w/ alpha");
        return false;
    }
    double ang_arg = acos(cos_of_arg);
    double q_s1a = alpha - ang_arg;
    //double q = q_abb + DH_q_offsets[i]; used for forward kin
    //q_s1-= DH_q_offsets[1]; // this is the only joint w/ non-zero offset
    double q_s1b = alpha + ang_arg;
    // have 2 candidate solns for q_s1 (a and b), but only 1 of them is correct
    // check for fit of 2 equations--one for w_x, other for w_y
    double LHSax = r_perp*cos(q_s1a)*cos(q_humerus) + Lw*sin(q_s1a); 
    double LHSbx = r_perp*cos(q_s1b)*cos(q_humerus) + Lw*sin(q_s1b);
    double RHSx = w_wrt_1(0);     
    
    double LHSay = r_perp*sin(q_s1a)*cos(q_humerus) - Lw*cos(q_s1a);
    double LHSby = r_perp*sin(q_s1b)*cos(q_humerus) - Lw*cos(q_s1b);
    double RHSy = w_wrt_1(1);
    
    double erra = fabs(LHSax-RHSx)+fabs(LHSay-RHSy);
    double errb = fabs(LHSbx-RHSx)+fabs(LHSby-RHSy);

    //ROS_INFO("test_a: LHS1 = %f; LHS2 = %f, RHS = %f",LHS1,LHS2,RHS);
    //ROS_INFO("test_b: LHS1b = %f; LHS2b = %f, RHSb = %f",LHS1b,LHS2b,RHSb); 
    //ROS_INFO("err_a, err_b= %f, %f",err1,err2);
    
    // assign qs1 to the smaller error:
    double err_qs = erra;
    q_s1 =q_s1a;
    if (errb<erra) {
        q_s1 = q_s1b;
        err_qs = errb;
    }
    //solved for DH angle q_s1; convert to BAXTER angle
    q_s1-= DH_q_offsets[1]; // this is the only joint w/ non-zero offset
    
    /*
    ROS_INFO("test fwd kin:");
    Eigen::Matrix4d    A_12 = compute_A_of_DH_approx(1, q_s1);    
    Eigen::Matrix4d    A_23 = compute_A_of_DH_approx(2, q_humerus); 
    Eigen::Matrix4d    A_34 = compute_A_of_DH_approx(3, q_elbow);
    Eigen::Matrix4d    A_45 = compute_A_of_DH_approx(4, 0);    
    Eigen::Matrix4d    A_15 = A_12*A_23*A_34*A_45;
    cout<<"A_15(q_s1,q_humerus,q_elbow) = "<<endl;
    cout<<A_15<<endl;   
     * */ 
}

    
//find the wrist solns to fit desired orientation, given q0 through q3.
// MUST EXPRESS R_des in right_arm_mount frame
// provide q0 through q3 in q_in, and populate q_solns w/ 0, 1 or 2 complete 7-dof solns
// note: if q5 (wrist bend) is near zero, then at a wrist singularity; 
// inf solutions of q4+D, q6-D
// use q0, q1, q2, q3 from q_in; copy these values to q_solns, and tack on the two solutions q4, q5, q6
bool Baxter_IK_solver::solve_spherical_wrist(Vectorq7x1 q_in,Eigen::Matrix3d R_des, std::vector<Vectorq7x1> &q_solns) {
    bool is_singular = false;
    bool at_least_one_valid_soln = false;
    Eigen::Matrix4d A01,A12,A23,A04;
    Eigen::Matrix4d  A45,A05,A56,A06;
    //A01 = compute_A_of_DH(0, q_in[0]);
    //A12 = compute_A_of_DH(1, q_in[1]);
    //A23 = compute_A_of_DH(2, q_in[2]);
    //A03 = A01*A12*A23;  
    fwd_kin_solve_(q_in);
    A04 = A_mat_products[3];
    Eigen::Vector3d n6,t6,b6; //axes of frame6; b6 is same as b_des
    Eigen::Vector3d n5,t5,b5; // axes of frame5;    
    Eigen::Vector3d n4,t4,b4; // axes of frame4

    Eigen::Vector3d n_des,b_des; // desired x-axis and z-axis of flange frame
    n4 = A04.col(0).head(3);
    t4 = A04.col(1).head(3);    
    b4 = A04.col(2).head(3);  
    b_des = R_des.col(2);
    n_des = R_des.col(0);
    b5 = b4.cross(b_des);
    double q4,q5,q6; //these are: q_forearm, q_wrist_bend, q_tool_flange_spin
    Vectorq7x1 q_soln;
    q_solns.clear();
    
      if (b5.norm() <= 0.000001) {
                q4=0;
                is_singular = true; // do something with this value
      }
      else {
            double cq4= b5.dot(-t4);
            double sq4= b5.dot(n4); 
            q4= atan2(sq4, cq4);
        }
    // choose the positive forearm-rotation solution:
    if (q4>M_PI) {
        q4-= 2*M_PI;
    }    
    if (q4<0.0) {
        q4+= M_PI;
    }

    // THESE OPTIONS LOOK GOOD FOR q4
    //std::cout<<"forearm rotation options: "<<q4<<", "<<q4b<<std::endl;
    
    // use the + q4 soln to find q5, q6
    A45 = compute_A_of_DH(4, q4);
    A05 = A04*A45;
    n5 = A05.col(0).head(3);
    t5 = A05.col(1).head(3); 
    double cq5 = b_des.dot(t5);
    double sq5 = b_des.dot(-n5);
    q5 = atan2(sq5,cq5) + M_PI; //+DH_q_offsets[5]; // add M_PI has a hack...
    //std::cout<<"wrist bend = "<<q5<<std::endl;

    //solve for q6
    A56 = compute_A_of_DH(5, q5);
    A06 = A05*A56;
    n6 = A06.col(0).head(3);
    t6 = A06.col(1).head(3);   
        
    double cq6=n_des.dot(-n6);
    double sq6=n_des.dot(-t6);
    q6 =atan2(sq6, cq6)+M_PI;
    //ROS_INFO("q4,q5,q6 = %f, %f, %f",q4,q5,q6);
    if (fit_q_to_range(q_lower_limits[4],q_upper_limits[4],q4)) {
        if (fit_q_to_range(q_lower_limits[5],q_upper_limits[5],q5)) {
            if (fit_q_to_range(q_lower_limits[6],q_upper_limits[6],q6)) {
                q_soln = q_in;
                q_soln[4] = q4;
                q_soln[5] = q5;
                q_soln[6] = q6;
                q_solns.push_back(q_soln);
                at_least_one_valid_soln = true;   
            }
        }
    }

    //2nd wrist soln: 
    q4 -= M_PI;
    q5 *= -1.0; // flip wrist opposite direction
    q6 += M_PI; // fix the periodicity later; 
    if (fit_q_to_range(q_lower_limits[4],q_upper_limits[4],q4)) {
        if (fit_q_to_range(q_lower_limits[5],q_upper_limits[5],q5)) {
            if (fit_q_to_range(q_lower_limits[6],q_upper_limits[6],q6)) {
                q_soln = q_in;
                q_soln[4] = q4;
                q_soln[5] = q5;
                q_soln[6] = q6;
                q_solns.push_back(q_soln);
                at_least_one_valid_soln = true;   
            }
        }
    }    
    
       // ROS_INFO("alt q4,q5,q6 = %f, %f, %f",q_soln[3],q_soln[4],q_soln[5]);
       
    return is_singular;
}

//given a 7dof input, return ONLY the wrist soln closest to the suggestion.
//useful for numerical iterations on IK w/ updates to q123
bool Baxter_IK_solver::update_spherical_wrist(Vectorq7x1 q_in,Eigen::Matrix3d R_des, Vectorq7x1 &q_precise) {
    std::vector<Vectorq7x1> q_solns;
    solve_spherical_wrist(q_in,R_des, q_solns); // expect 2 wrist solns, if w/in jnt ranges
    int nsolns = q_solns.size();
    if (nsolns==0) { // no wrist solns within range
        q_precise = q_in; // just echo back the input
        return false; // note that we do not have a satisfactory soln
    }
    if (nsolns==2) { //ideal case--we have two choices.  One of them is correct, given q_forearm estimate
        //decide which soln is the one we want
        double err1 = (q_in - q_solns[0]).norm();
        double err2 = (q_in - q_solns[1]).norm();
        if (err1<err2) {
            q_precise = q_solns[0];
        }
        else {
            q_precise= q_solns[1];
        }            
            return true; // found a good soln
    }
    //trickier...if only 1 soln, 
    if (nsolns==1) {
        q_precise = q_solns[0];
        double err = (q_in - q_precise).norm();
        if (err> 1.0) //arbitrary threshold; should put in header
        {  ROS_WARN("update_spherical_wrist: likely poor fit");
        return false;
        }
    }
    ROS_WARN("update_spherical_wrist: unexpected case");
    return false; // don't know what to do with this case--should not happen
}

//this fnc requires a good, 7dof approximate soln for q_in 
// assumes Eigen::Affine3d const& desired_hand_pose is expressed w/rt right-arm mount frame (not torso frame)
// returns an improved 7dof soln in q_7dof_precise
bool Baxter_IK_solver::improve_7dof_soln(Eigen::Affine3d const& desired_hand_pose_wrt_arm_mount, Vectorq7x1 q_in, Vectorq7x1 &q_7dof_precise) {
    Eigen::Matrix3d R_flange_wrt_right_arm_mount = desired_hand_pose_wrt_arm_mount.linear();
    Vectorq7x1 q123_precise;
    
    double w_err_norm= precise_soln_q123(desired_hand_pose_wrt_arm_mount,q_in, q123_precise);
    bool valid = update_spherical_wrist(q123_precise,R_flange_wrt_right_arm_mount, q_7dof_precise);
    return valid;
}
