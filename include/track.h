#pragma once

#include "KalmanFilter.h"
#include <deque>

enum TrackState {
    New = 0,
    Tracked,
    Lost,
    LongLost,
    Removed
};

class Track {
public:
    bool is_activated;
    int track_id;
    int state;

    int frame_id;
    int tracklet_len;
    int start_frame;

    std::vector<float> det_tlwh;

    KFStateSpaceVec mean;
    KFStateSpaceMatrix covariance;

private:
    std::vector<float> _tlwh;
    std::vector<std::pair<uint8_t, float>> _class_hist;
    float _score;
    uint8_t _class_id;
    static constexpr float _alpha = 0.9;

    int _feat_history_size;
    FeatureVector _curr_feat, _smooth_feat;
    std::deque<FeatureVector> _feat_history;

    byte_kalman::KalmanFilter _kalman_filter;


public:
    /**
     * @brief Construct a new Track object
     * 
     * @param xywh Detection bounding box (xmid, ymid, width, height)
     * @param score Detection score
     * @param class_id Detection class ID
     * @param feat (Optional) Detection feature vector
     * @param feat_history_size Size of the feature history (default: 50)
     */
    Track(std::vector<float> tlwh, float score, uint8_t class_id, std::optional<FeatureVector> feat = std::nullopt, int feat_history_size = 50);
    ~Track();

    /**
     * @brief Get the next track ID
     * 
     * @return int Next track ID
     */
    int next_id();

    /**
     * @brief Get end frame-id of the track
     * 
     * @return int End frame-id of the track
     */
    int end_frame();

    /**
     * @brief Upates the track state to Lost
     * 
     */
    void mark_lost();

    /**
     * @brief Upates the track state to LongLost
     * 
     */
    void mark_long_lost();

    /**
     * @brief Upates the track state to Removed
     * 
     */
    void mark_removed();

    /**
     * @brief Activates the track
     * 
     * @param kalman_filter Kalman filter object for the track
     * @param frame_id Current frame-id
     */
    void activate(byte_kalman::KalmanFilter &kalman_filter, int frame_id);


    /**
     * @brief Re-activates an old track with the new track data
     * 
     * @param new_track New track object to be used to reactive the old track
     * @param frame_id Current frame-id
     * @param new_id Whether to assign a new ID to the track (default: false)
     */
    void re_activate(Track &new_track, int frame_id, bool new_id = false);

    /**
     * @brief Predict the next state of the track using the Kalman filter
     * 
     */
    void predict();

    /**
     * @brief Predict the next state of multiple tracks using the Kalman filter
     * 
     * @param tracks Tracks on which to perform the prediction step
     * @param kalman_filter Kalman filter object for the tracks
     */
    void static multi_predict(std::vector<Track *> &tracks, const byte_kalman::KalmanFilter &kalman_filter);

    /**
     * @brief Update the track state using the new detection
     * 
     * @param new_track New track object to be used to update the old track
     * @param frame_id Current frame-id
     */
    void update(Track &new_track, int frame_id);

private:
    /**
     * @brief Updates visual feature vector and feature history
     * Done by using a weighted average of the current feature vector and the previous feature vector
     * 
     * @param feat Current feature vector
     */
    void _update_features(FeatureVector &feat);

    /**
     * @brief Populate a DetVec bbox object (xywh) from the detection bounding box (tlwh)
     * 
     * @param bbox_xywh DetVec bbox object (xywh) to be populated
     * @param tlwh Detection bounding box (tlwh)
     */
    void _populate_DetVec_xywh(DetVec &bbox_xywh, const std::vector<float> &tlwh);

    /**
     * @brief Update the tracklet bounding box (stored as tlwh) inplace accoding to the tracker state
     * 
     */
    void _update_tracklet_tlwh_inplace();

    /**
     * @brief Update the class_id for the given tracklet on the basis of the frequency of the class_id in the class history
     * The history maintains a list of (class_id, score) pairs.
     * If a particular class_id occurs with a higher score, multiple times, it is more likely to be the correct class_id
     * If a particular class_id occurs with a lower score, multiple times, it is less likely to be the correct class_id
     * 
     * The frequency of the class_id is calculated as the sum of the scores of the class_id in the history
     * 
     * @param class_id Current class_id for the bounding box
     * @param score Current score for the bounding box
     */
    void _update_class_id(uint8_t class_id, float score);
};