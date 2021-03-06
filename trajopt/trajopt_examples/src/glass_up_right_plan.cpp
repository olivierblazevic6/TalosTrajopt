/**
 * @file glass_up_right_plan.cpp
 * @brief Example using Trajopt for constrained free space planning
 *
 * @author Levi Armstrong
 * @date Dec 18, 2017
 * @version TODO
 * @bug No known bugs
 *
 * @copyright Copyright (c) 2017, Southwest Research Institute
 *
 * @par License
 * Software License Agreement (Apache License)
 * @par
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * @par
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <trajopt_utils/macros.h>
TRAJOPT_IGNORE_WARNINGS_PUSH
#include <jsoncpp/json/json.h>
#include <ros/ros.h>
#include <srdfdom/model.h>
#include <urdf_parser/urdf_parser.h>
TRAJOPT_IGNORE_WARNINGS_POP
#include <tesseract_ros/kdl/kdl_chain_kin.h>
#include <tesseract_ros/kdl/kdl_env.h>
#include <tesseract_ros/ros_basic_plotting.h>
#include <trajopt/plot_callback.hpp>
#include <trajopt/file_write_callback.hpp>
#include <trajopt/problem_description.hpp>
#include <trajopt_utils/config.hpp>
#include <trajopt_utils/logging.hpp> trajopt::gLogLevel = util::LevelWarn;
#include "trajopt_examples/my_msg.h"
#include <geometry_msgs/Point.h>
using namespace trajopt;
using namespace tesseract;

const std::string ROBOT_DESCRIPTION_PARAM = "robot_description"; /**< Default ROS parameter for robot description */
const std::string ROBOT_SEMANTIC_PARAM = "robot_description_semantic"; /**< Default ROS parameter for robot*/
const std::string TRAJOPT_DESCRIPTION_PARAM ="trajopt_description"; /**< Default ROS parameter for trajopt description */

static bool plotting_ = false;
static bool write_to_file_ = false;
static int steps_ = 10  ; // nombre d'étapes dans la traj du robot (!= du nombres d'itérations)
static std::string method_ = "cpp"; // méthode Json ou Cpp (en l'occurence on prend cpp)
static urdf::ModelInterfaceSharedPtr urdf_model_; /**< URDF Model */
static srdf::ModelSharedPtr srdf_model_;          /**< SRDF Model */
static tesseract_ros::KDLEnvPtr env_;             /**< Trajopt Basic Environment */
double sign = 1; //on determine la direction dans la quelle ira le bras
float x_sphere,y_sphere,z_sphere;

void Callback(const geometry_msgs::Point::ConstPtr& msg) {
    x_sphere = msg->x;    
    y_sphere = msg->y;
    z_sphere = msg->z;  
    ROS_INFO( "%.3f,%.3f,%.3f",msg->x ,msg->y,msg->z) ;
}

TrajOptProbPtr cppMethod()  //la bonne méthode : cpp
{
  ProblemConstructionInfo pci(env_);
  ROS_ERROR("Ou est l'erreur");
  // Populate Basic Info
  pci.basic_info.n_steps = steps_;
  pci.basic_info.manip = "manipulator";
  pci.basic_info.start_fixed = false;
  pci.basic_info.use_time = false;
  ROS_ERROR("Ou est l'erreur");

  // Create Kinematic Object
  pci.kin = pci.env->getManipulator(pci.basic_info.manip); //renvoie nullptr, non nécessairement interessant à avoir pour creer un objet. Il doit y avoir un probleme ici 
  ROS_ERROR("Ou est l'erreur");

  // Populate Init Info
  Eigen::VectorXd start_pos = pci.env->getCurrentJointValues(pci.kin->getName()); //ligne problématique, on ne rentre pas dans getCurrentJointValues. problème avec pci.kin->getName()? cf l82
  ROS_ERROR("Ou est l'erreur");

  Eigen::VectorXd end_pos;
  ROS_ERROR("Ou est l'erreur");

  end_pos.resize(pci.kin->numJoints()); //également
  ROS_ERROR("Ou est l'erreur");

  end_pos << 0.0 , 0.0, 0.0, sign*0.1, 0.0, 0.0, 0.0; //POSITION FINALE A ne pas MODIFIER A SOUHAIT.  
  ROS_ERROR("Ou est l'erreur");

  pci.init_info.type = InitInfo::GIVEN_TRAJ;
  pci.init_info.data = TrajArray(steps_, pci.kin->numJoints());// aussi
  for (unsigned idof = 0; idof < pci.kin->numJoints(); ++idof)
  {
    pci.init_info.data.col(idof) = Eigen::VectorXd::LinSpaced(steps_, start_pos[idof], end_pos[idof]);
  }
  ROS_ERROR("Ou est l'erreur");
  
  // Populate Cost Info
  std::shared_ptr<JointVelTermInfo> jv = std::shared_ptr<JointVelTermInfo>(new JointVelTermInfo);
  jv->coeffs = std::vector<double>(7, 1.0);
  jv->targets = std::vector<double>(7, 0.0);
  jv->first_step = 0;
  jv->last_step = pci.basic_info.n_steps - 1;
  jv->name = "joint_vel";
  jv->term_type = TT_COST;
  pci.cost_infos.push_back(jv);
  ROS_ERROR("Ou est l'erreur");

  std::shared_ptr<CollisionTermInfo> collision = std::shared_ptr<CollisionTermInfo>(new CollisionTermInfo);
  collision->name = "collision";
  collision->term_type = TT_COST;
  collision->continuous = false;
  collision->first_step = 0;
  collision->last_step = pci.basic_info.n_steps - 1;
  collision->gap = 1;
  collision->info = createSafetyMarginDataVector(pci.basic_info.n_steps, 0.025, 20); // trajectoire très satisfaisante pour param2=0.001, mais bcp trop lent.
  for (auto& info : collision->info)
  {
    info->SetPairSafetyMarginData("base_link", "link_5", 0.01, 10);
    info->SetPairSafetyMarginData("link_3", "link_5", 0.01, 10);
    info->SetPairSafetyMarginData("link_3", "link_6", 0.01, 10);
  }
  pci.cost_infos.push_back(collision);

  // Populate Constraints
  double delta = 0.2 / pci.basic_info.n_steps;
  for (auto i = 0; i < pci.basic_info.n_steps; ++i)
  {
    std::shared_ptr<CartPoseTermInfo> pose = std::shared_ptr<CartPoseTermInfo>(new CartPoseTermInfo);
    pose->term_type = TT_CNT;
    pose->name = "waypoint_cart_" + std::to_string(i);
    pose->link = "wrist_left_ft_tool_link";
    
    pose->timestep = i;
    pose->xyz = Eigen::Vector3d(0.0,sign*(-0.1 + delta * i), 0);
    pose->wxyz = Eigen::Vector4d(0.0, 0.0, 1.0, 0.0);
    
    if (i == (pci.basic_info.n_steps - 1) || i == 0)
    {
      pose->pos_coeffs = Eigen::Vector3d(10, 10, 10);
      pose->rot_coeffs = Eigen::Vector3d(0, 0, 0);  //CONTRAINTES SUR LA ROTATION
    }
    else
    {
      pose->pos_coeffs = Eigen::Vector3d(0, 0, 0);
      pose->rot_coeffs = Eigen::Vector3d(0, 0, 0);  //CONTRAINTES SUR LA ROTATION
    }
    pci.cnt_infos.push_back(pose);
  }
  ROS_ERROR("Ou est l'erreur");

  return ConstructProblem(pci);
}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "glass_up_right_plan");
  ros::NodeHandle pnh("~");
  ros::NodeHandle nh;
  ros::Subscriber sub = nh.subscribe("my_topic", 1, Callback);
  

  float xverif= 1;//test, ne pas juger le code barbare 
  float yverif= 1; //(on force le passage dans le premier if afin d'initialiser la sphère)
  float zverif= 1;

beginning:
  // Initial setup
  std::string urdf_xml_string, srdf_xml_string;
  nh.getParam(ROBOT_DESCRIPTION_PARAM, urdf_xml_string);
  nh.getParam(ROBOT_SEMANTIC_PARAM, srdf_xml_string);
  urdf_model_ = urdf::parseURDF(urdf_xml_string);

  srdf_model_ = srdf::ModelSharedPtr(new srdf::Model);
  srdf_model_->initString(*urdf_model_, srdf_xml_string);

  env_ = tesseract_ros::KDLEnvPtr(new tesseract_ros::KDLEnv);
  assert(urdf_model_ != nullptr);
  assert(env_ != nullptr);
  
  bool success = env_->init(urdf_model_, srdf_model_);
  assert(success);
  // Create Plotting tool
  tesseract_ros::ROSBasicPlottingPtr plotter(new tesseract_ros::ROSBasicPlotting(env_));  


  
  // Creation de la sphere
    AttachableObjectPtr obj(new AttachableObject());
    std::shared_ptr<shapes::Sphere> sphere(new shapes::Sphere());
    Eigen::Isometry3d sphere_pose;

    sphere->radius = 0.15; // rayon de la sphère rayon init=0,15
    sphere_pose.setIdentity();

    sphere_pose.translation() = Eigen::Vector3d(x_sphere,y_sphere,z_sphere);  // coordonnées de la sphere selon x y z  posinit=(0.5, 0, 0.55)


    obj->name = "sphere_attached";
    obj->visual.shapes.push_back(sphere);
    obj->visual.shape_poses.push_back(sphere_pose);
    obj->collision.shapes.push_back(sphere);
    obj->collision.shape_poses.push_back(sphere_pose);
    obj->collision.collision_object_types.push_back(CollisionObjectType::UseShapeType);  

    env_->addAttachableObject(obj);

    AttachedBodyInfo attached_body;
    attached_body.object_name = "sphere_attached";
    attached_body.parent_link_name = "base_link";
    attached_body.transform.setIdentity();
    // 
    attached_body.touch_links = {"base_link","link_1","link_2","link_3","link_","link_"}; // This element enables the attached body
    //  to collide with other links ATTENTION A MODIFIER EVENTUELLEMENT

    env_->attachBody(attached_body);

  // Get ROS Parameters
  pnh.param("plotting", plotting_, plotting_);
  pnh.param("write_to_file", write_to_file_, write_to_file_);
  pnh.param<std::string>("method", method_, method_);
  pnh.param<int>("steps", steps_, steps_);



while(ros::ok())
  
  if ((xverif,yverif,zverif)!= (x_sphere,y_sphere,z_sphere)) //on vérifie que pos_réelle==pos_souhaitée
  {
    xverif= x_sphere;
    yverif= y_sphere;
    zverif= z_sphere; 
    goto beginning;
  }
  else{
    if (sign ==1){
    
     
      std::unordered_map<std::string, double> ipos;
     

      ipos["arm_left_1_joint"] = 0;
      ipos["arm_left_2_joint"] = 0;
      ipos["arm_left_3_joint"] = 0.0;
      ipos["arm_left_4_joint"] = -0.1;
      ipos["arm_left_5_joint"] = 0.0;
      ipos["arm_left_6_joint"] = 0;
      ipos["arm_left_7_joint"] = 0.0;

      ROS_ERROR("you're here");


      //end_pos << 0.4, 0.2762, 0.0, -1.3348, 0.0, 1.4959, 0.0;
      env_->setState(ipos);
      ROS_ERROR("you're here");

      plotter->plotScene();
      ROS_ERROR("you're here");

      // Set Log Level
      util::gLogLevel = util::LevelInfo;
      // Setup Problem
      ROS_ERROR("you're here");

      TrajOptProbPtr prob;
      if (method_ == "cpp")
        prob = cppMethod();
      ROS_ERROR("you're here");
    
    
    

      // Solve Trajectory
      ROS_INFO("glass upright plan example");

      std::vector<tesseract::ContactResultMap> collisions;
      ContinuousContactManagerBasePtr manager = prob->GetEnv()->getContinuousContactManager();
      manager->setActiveCollisionObjects(prob->GetKin()->getLinkNames());
      manager->setContactDistanceThreshold(0);
      ROS_ERROR("you're here");

      bool found = tesseract::continuousCollisionCheckTrajectory(
          *manager, *prob->GetEnv(), *prob->GetKin(), prob->GetInitTraj(), collisions);

      ROS_INFO((found) ? ("Initial trajectory is in collision") : ("Initial trajectory is collision free"));
      ROS_ERROR("you're here");

      sco::BasicTrustRegionSQP opt(prob);
      if (plotting_)
      {
        opt.addCallback(PlotCallback(*prob, plotter));
      }
      ROS_ERROR("you're here");

      std::shared_ptr<std::ofstream> stream_ptr;
      if (write_to_file_)
      {
        // Create file write callback discarding any of the file's current contents
        stream_ptr.reset(new std::ofstream);
        std::string path = ros::package::getPath("trajopt") + "/scripts/glass_up_right_plan.csv";
        stream_ptr->open(path, std::ofstream::out | std::ofstream::trunc);
        opt.addCallback(trajopt::WriteCallback(stream_ptr, prob));
      } 
      ROS_ERROR("you're here");

      opt.initialize(trajToDblVec(prob->GetInitTraj()));
      ros::Time start= ros::Time::now();
      ROS_ERROR("you've got far");

      opt.optimize();//la baguette magique ? la baguette magique. /home/blaz/optimized_planning_ws/src/trajopt/trajopt_sco/src/optimizers.cpp
      ROS_ERROR("soon over");

      ROS_ERROR("%.3f",(ros::Time::now()-start).toSec());

      double d = 0;
      TrajArray traj = getTraj(opt.x(), prob->GetVars());
      
      for (unsigned i = 1; i < traj.rows(); ++i)
      {
        for (unsigned j = 0; j < traj.cols(); ++j)
        {
          d += std::abs(traj(i, j) - traj(i - 1, j));
        }
      }
      ROS_ERROR("trajectory norm: %.3f", d);
      if (plotting_)
      {
      plotter->clear();
      }
      if (write_to_file_)
      {
        stream_ptr->close();
        ROS_INFO("Data written to file. Evaluate using scripts in trajopt/scripts.");
      }
      collisions.clear();
      found = tesseract::continuousCollisionCheckTrajectory(
          *manager, *prob->GetEnv(), *prob->GetKin(), prob->GetInitTraj(), collisions);

      ROS_INFO((found) ? ("Final trajectory is in collision") : ("Final trajectory is collision free"));
      sign =-1;     
      ROS_ERROR("you even finished");

      ros::spinOnce();

      ROS_ERROR("Press ENTER twice to start calculating a new trajectory");
      getchar();

    }
    else{
    
     
      std::unordered_map<std::string, double> ipos;
     

      ipos["arm_left_1_joint"] = 0;
      ipos["arm_left_2_joint"] = 0;
      ipos["arm_left_3_joint"] = 0.0;
      ipos["arm_left_4_joint"] = 0.1;
      ipos["arm_left_5_joint"] = 0.0;
      ipos["arm_left_6_joint"] = 0;
      ipos["arm_left_7_joint"] = 0.0;


      ROS_ERROR("you're here");


      //end_pos << 0.4, 0.2762, 0.0, -1.3348, 0.0, 1.4959, 0.0;
      env_->setState(ipos);
      ROS_ERROR("you're here");

      plotter->plotScene();
      ROS_ERROR("you're here");

      // Set Log Level
      util::gLogLevel = util::LevelInfo;
      // Setup Problem
      ROS_ERROR("you're here");

      TrajOptProbPtr prob;
      if (method_ == "cpp")
        prob = cppMethod();
      ROS_ERROR("you're here");
    
    
    

      // Solve Trajectory
      ROS_INFO("glass upright plan example");

      std::vector<tesseract::ContactResultMap> collisions;
      ContinuousContactManagerBasePtr manager = prob->GetEnv()->getContinuousContactManager();
      manager->setActiveCollisionObjects(prob->GetKin()->getLinkNames());
      manager->setContactDistanceThreshold(0);
      ROS_ERROR("you're here");

      bool found = tesseract::continuousCollisionCheckTrajectory(
          *manager, *prob->GetEnv(), *prob->GetKin(), prob->GetInitTraj(), collisions);

      ROS_INFO((found) ? ("Initial trajectory is in collision") : ("Initial trajectory is collision free"));
      ROS_ERROR("you're here");

      sco::BasicTrustRegionSQP opt(prob);
      if (plotting_)
      {
        opt.addCallback(PlotCallback(*prob, plotter));
      }
      ROS_ERROR("you're here");

      std::shared_ptr<std::ofstream> stream_ptr;
      if (write_to_file_)
      {
        // Create file write callback discarding any of the file's current contents
        stream_ptr.reset(new std::ofstream);
        std::string path = ros::package::getPath("trajopt") + "/scripts/glass_up_right_plan.csv";
        stream_ptr->open(path, std::ofstream::out | std::ofstream::trunc);
        opt.addCallback(trajopt::WriteCallback(stream_ptr, prob));
      } 
      ROS_ERROR("you're here");

      opt.initialize(trajToDblVec(prob->GetInitTraj()));
      ros::Time start= ros::Time::now();
      ROS_ERROR("you've got far");

      opt.optimize();//la baguette magique ? la baguette magique. /home/blaz/optimized_planning_ws/src/trajopt/trajopt_sco/src/optimizers.cpp
      ROS_ERROR("soon over");

      ROS_ERROR("%.3f",(ros::Time::now()-start).toSec());

      double d = 0;
      TrajArray traj = getTraj(opt.x(), prob->GetVars());
      
      for (unsigned i = 1; i < traj.rows(); ++i)
      {
        for (unsigned j = 0; j < traj.cols(); ++j)
        {
          d += std::abs(traj(i, j) - traj(i - 1, j));
        }
      }
      ROS_ERROR("trajectory norm: %.3f", d);
      if (plotting_)
      {
      plotter->clear();
      }
      if (write_to_file_)
      {
        stream_ptr->close();
        ROS_INFO("Data written to file. Evaluate using scripts in trajopt/scripts.");
      }
      collisions.clear();
      found = tesseract::continuousCollisionCheckTrajectory(
          *manager, *prob->GetEnv(), *prob->GetKin(), prob->GetInitTraj(), collisions);

      ROS_INFO((found) ? ("Final trajectory is in collision") : ("Final trajectory is collision free"));
      sign =1;     
      ROS_ERROR("you even finished");

      ros::spinOnce();

      ROS_ERROR("Press ENTER twice to start calculating a new trajectory");
      getchar();

    }

    
  }
  return 0;
}
