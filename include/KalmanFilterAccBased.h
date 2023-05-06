#pragma once

#include "DataType.h"

namespace kalman_modified {
class KalmanFilter {
public:
    static const double chi2inv95[10];

    /**
     * @brief Construct a new Kalman Filter object.
     * 
     * @param dt Time interval between consecutive measurements (dt = 1/FPS). Defaults to 1.0.
     */
    KalmanFilter(double dt);

    /**
     * @brief Initialize the Kalman Filter with a measurement (detection).
     * 
     * @param det Detection [x, y, w, h].
     * @return KFDataStateSpace Kalman filter state space data [mean, covariance].
     */
    KFDataStateSpace init(const DetVec &det);

    /**
     * @brief Predict the next Kalman Filter state space data (mean, covariance) given the current state space data.
     * 
     * @param mean Current Kalman Filter state space mean.
     * @param covariance Current Kalman Filter state space covariance.
     */
    void predict(KFStateSpaceVec &mean, KFStateSpaceMatrix &covariance);

    /**
     * @brief Project the Kalman Filter state space data (mean, covariance) to measurement space.
     * 
     * @param mean Kalman Filter state space mean.
     * @param covariance Kalman Filter state space covariance.
     * @return KFDataMeasurementSpace Kalman Filter measurement space data [mean, covariance]. 
     */
    KFDataMeasurementSpace project(const KFStateSpaceVec &mean, const KFStateSpaceMatrix &covariance);

    /**
     * @brief Update the Kalman Filter state space data (mean, covariance) given the measurement (detection).
     * 
     * @param mean Kalman Filter state space mean.
     * @param covariance Kalman Filter state space covariance.
     * @param measurement Detection [x, y, w, h].
     * @return KFDataStateSpace Updated Kalman Filter state space data [mean, covariance].
     */
    KFDataStateSpace update(const KFStateSpaceVec &mean, const KFStateSpaceMatrix &covariance, const DetVec &measurement);


    /**
     * @brief Compute the gating distance between the Kalman Filter state space data (mean, covariance) and the 
     * measurements (detections) using the Mahalanobis distance.
     * 
     * @param mean Kalman Filter state space mean.
     * @param covariance Kalman Filter state space covariance.
     * @param measurements Detections [x, y, w, h].
     * @return Eigen::Matrix<float, 1, Eigen::Dynamic> Gating distance.
     */
    Eigen::Matrix<float, 1, Eigen::Dynamic> gating_distance(
            const KFStateSpaceVec &mean,
            const KFStateSpaceMatrix &covariance,
            const std::vector<DetVec> &measurements);

private:
    /**
     * @brief Initialize Kalman Filter matrices (state transition, measurement, process noise covariance).
     * 
     * @param dt Time interval between consecutive measurements (dt = 1/FPS).
     */
    void _init_kf_matrices(double dt);

    float _init_pos_weight, _init_vel_weight;
    float _std_factor_acceleration, _std_offset_acceleration;
    float _std_factor_detection, _min_std_detection;
    float _std_factor_motion_compensated_detection, _min_std_motion_compensated_detection;
    float _velocity_coupling_factor;
    uint8_t _velocity_half_life;

    Eigen::Matrix<float, KALMAN_STATE_SPACE_DIM, KALMAN_STATE_SPACE_DIM, Eigen::RowMajor> _state_transition_matrix;
    Eigen::Matrix<float, KALMAN_MEASUREMENT_SPACE_DIM, KALMAN_STATE_SPACE_DIM, Eigen::RowMajor> _measurement_matrix;
    Eigen::Matrix<float, KALMAN_STATE_SPACE_DIM, KALMAN_STATE_SPACE_DIM, Eigen::RowMajor> _process_noise_covariance;
};
}// namespace kalman_modified
