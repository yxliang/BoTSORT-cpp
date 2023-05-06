#include "KalmanFilter.h"
#include <eigen3/Eigen/Cholesky>

namespace byte_kalman {
const double KalmanFilter::chi2inv95[10] = {
        0,
        3.8415,
        5.9915,
        7.8147,
        9.4877,
        11.070,
        12.592,
        14.067,
        15.507,
        16.919};

KalmanFilter::KalmanFilter(double dt = 1.0) {
    _init_kf_matrices(dt);

    _std_weight_position = 1.0 / 20;
    _std_weight_velocity = 1.0 / 160;
}

void KalmanFilter::_init_kf_matrices(double dt) {
    _measurement_matrix = Eigen::MatrixXf::Identity(KALMAN_MEASUREMENT_SPACE_DIM, KALMAN_STATE_SPACE_DIM);

    _state_transition_matrix = Eigen::MatrixXf::Identity(KALMAN_STATE_SPACE_DIM, KALMAN_STATE_SPACE_DIM);
    for (uint8_t i = 0; i < 4; i++) {
        _state_transition_matrix(i, i + 4) = dt;
    }
}

KFDataStateSpace KalmanFilter::init(const DetVec &measurement) {
    constexpr float init_velocity = 0.0;
    KFStateSpaceVec mean_state_space;

    for (uint8_t i = 0; i < KALMAN_STATE_SPACE_DIM; i++) {
        if (i < 4) {
            mean_state_space(i) = measurement(i);
        } else {
            mean_state_space(i) = init_velocity;
        }
    }

    float w = measurement(2), h = measurement(3);
    KFStateSpaceVec std;
    for (uint8_t i = 0; i < KALMAN_STATE_SPACE_DIM; i++) {
        if (i < 4) {
            std(i) = 2 * _std_weight_position * (i % 2 == 0 ? w : h);
        } else {
            std(i) = 10 * _std_weight_velocity * (i % 2 == 0 ? w : h);
        }
    }
    KFStateSpaceVec std_squared = std.array().square();
    KFStateSpaceMatrix covariance = std_squared.asDiagonal();
    return std::make_pair(mean_state_space, covariance);
}

void KalmanFilter::predict(KFStateSpaceVec &mean, KFStateSpaceMatrix &covariance) {
    Eigen::VectorXf std_pos(KALMAN_MEASUREMENT_SPACE_DIM);
    std_pos << _std_weight_position * mean(2),
            _std_weight_position * mean(3),
            _std_weight_position * mean(2),
            _std_weight_position * mean(3);

    Eigen::VectorXf std_vel(4);
    std_vel << _std_weight_velocity * mean(2),
            _std_weight_velocity * mean(3),
            _std_weight_velocity * mean(2),
            _std_weight_velocity * mean(3);

    Eigen::VectorXf std_combined;
    std_combined.block(0, 0, 4, 1) = std_pos;
    std_combined.block(4, 0, 4, 1) = std_vel;
    std_combined = std_combined.array().square();
    KFStateSpaceMatrix motion_cov = std_combined.asDiagonal();

    mean = _state_transition_matrix * mean;
    covariance = _state_transition_matrix * covariance * _state_transition_matrix.transpose() + motion_cov;
}

KFDataMeasurementSpace KalmanFilter::project(const KFStateSpaceVec &mean, const KFStateSpaceMatrix &covariance) {
    Eigen::VectorXf innovation_cov(KALMAN_MEASUREMENT_SPACE_DIM);
    innovation_cov << _std_weight_position * mean(2),
            _std_weight_position * mean(3),
            _std_weight_position * mean(2),
            _std_weight_position * mean(3);
    innovation_cov = innovation_cov.array().square();
    innovation_cov = innovation_cov.asDiagonal();

    KFMeasSpaceVec mean_updated = _measurement_matrix * mean;
    KFMeasSpaceMatrix covariance_updated = _measurement_matrix * covariance * _measurement_matrix.transpose() + innovation_cov;
    return std::make_pair(mean_updated, covariance_updated);
}

KFDataStateSpace KalmanFilter::update(const KFStateSpaceVec &mean, const KFStateSpaceMatrix &covariance, const DetVec &measurement) {
    KFDataMeasurementSpace projected = project(mean, covariance);
    KFMeasSpaceVec projected_mean = projected.first;
    KFMeasSpaceMatrix projected_covariance = projected.second;

    Eigen::Matrix<float, KALMAN_MEASUREMENT_SPACE_DIM, KALMAN_STATE_SPACE_DIM> B = (covariance * _measurement_matrix.transpose()).transpose();
    Eigen::Matrix<float, KALMAN_STATE_SPACE_DIM, KALMAN_MEASUREMENT_SPACE_DIM> kalman_gain = (projected_covariance.llt().solve(B)).transpose();
    Eigen::Matrix<float, 1, KALMAN_MEASUREMENT_SPACE_DIM> innovation = measurement - projected_mean;

    KFStateSpaceVec mean_updated = mean + innovation * kalman_gain.transpose();
    KFStateSpaceMatrix covariance_updated = covariance - kalman_gain * projected_covariance * kalman_gain.transpose();
    return std::make_pair(mean_updated, covariance_updated);
}

Eigen::Matrix<float, 1, Eigen::Dynamic> KalmanFilter::gating_distance(
        const KFStateSpaceVec &mean,
        const KFStateSpaceMatrix &covariance,
        const std::vector<DetVec> &measurements) {
    KFDataMeasurementSpace projected = this->project(mean, covariance);
    KFMeasSpaceVec projected_mean = projected.first;
    KFMeasSpaceMatrix projected_covariance = projected.second;

    KFMeasSpaceVec diff(measurements.size(), 4);
    for (uint8_t i = 0; i < measurements.size(); i++) {
        diff.row(i) = measurements[i] - projected_mean;
    }

    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> cholesky_factor = projected_covariance.llt().matrixL();
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> mahalanobis_distance = cholesky_factor.triangularView<Eigen::Lower>().solve(diff).transpose();
    Eigen::Matrix<float, 1, Eigen::Dynamic> distance;

    auto mahalanobis_distance_squared = mahalanobis_distance.array().square().matrix();
    return mahalanobis_distance_squared.rowwise().sum();
}
}// namespace byte_kalman
