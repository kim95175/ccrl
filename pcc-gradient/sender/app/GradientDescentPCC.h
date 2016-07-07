#ifndef __GRADIENT_DESCENT_PCC_H__
#define __GRADIENT_DESCENT_PCC_H__

#include "pcc.h"

class GradientDescentPCC: public PCC {
public:
	GradientDescentPCC() : first_(true), up_utility_(0), down_utility_(0), seq_large_incs_(0), consecutive_big_changes_(0), trend_count_(0), decision_count_(0), curr_(0), prev_change_(0), kRobustness(2),next_delta(0) {}

protected:
	virtual void search() {
		guess();
	}
		
	bool ensure_min_change() {
		/*
		if ((prev_change_ >= 0) && (prev_change_ < kMinRateMbps / 2) && (base_rate_ < 3 * kMinRateMbps)) prev_change_ = kMinRateMbps / 2;
		else if ((prev_change_ < 0) && (prev_change_ > -1 * kMinRateMbps / 2) && (base_rate_ < 3 * kMinRateMbps)) prev_change_ = -1 * kMinRateMbps / 2;
		else if ((prev_change_ >= 0) && (prev_change_ < 2 * kMinRateMbps) && (base_rate_ < 5 * kMinRateMbps)) prev_change_ = 2 * kMinRateMbps;
		else if ((prev_change_ < 0) && (prev_change_ > -2 * kMinRateMbps) && (base_rate_ < 5 * kMinRateMbps)) prev_change_ = -2 * kMinRateMbps;
		*/
		//return false;
		
		
		if ((prev_change_ >= 0) && (prev_change_ < 0.005 * base_rate_)) {
			prev_change_ = 0.005 * base_rate_;
			return true;
		}
		if ((prev_change_ < 0) && (prev_change_ > -0.005 * base_rate_)) {
			prev_change_ = -0.005 * base_rate_;
			return true;
		}
		
		
		if ((prev_change_ >= 0) && (prev_change_ > 0.1 * base_rate_)) {
			prev_change_ = 0.1 * base_rate_;
			return true;
		}
		if ((prev_change_ < 0) && (prev_change_ < -0.1 * base_rate_)) {
			prev_change_ = -0.1 * base_rate_;
			return true;
		}
		return false;
	} 
	
	virtual double delta_for_base_rate() {
		if (base_rate_ < 1) return 0.3;
		else if (base_rate_ < 2) return 0.25;
		else if (base_rate_ < 3) return 0.2; 
		else if (base_rate_ < 10) return 0.1;
		else return 0.05;
	}
	
	virtual void do_last_change() {
		static int total = 0;
		static int enforced = 0;
		if (ensure_min_change()) {
			enforced++;
		}
		total++;
		
		//if (total % 10 == 0) cout << "Enforced change: " << (double) enforced / total << endl;
		base_rate_ += prev_change_;
		//cout << "Gradient: " << prev_change_ << " new base rate:" << base_rate_ << endl;
		if (base_rate_ * (1 - delta_for_base_rate()) < kMinRateMbps) {
			go_to_slow_start();
			//base_rate_ = kMinRateMbps * (1 + delta_for_base_rate());
		}
		setRate(base_rate_);
	}
	
	virtual void decide(long double start_utility, long double end_utility, bool timeout) {
		double gradient = -1 * (start_utility - end_utility) / (2 * delta_for_base_rate());
		prev_gradiants_[curr_] = gradient;

		trend_count_++;
		curr_ = (curr_ + 1) % 100;
		if ((trend_count_ < kRobustness) && (!timeout)) {
			return;
		}
		trend_count_ = 0;
		
		double change = epsilon() * avg_gradient();	
		if (change * prev_change_ >= 0) decision_count_++;
		else decision_count_ = 0;
		
		//cout << "multiplier: " << (pow(decision_count_, 2) + 1) * epsilon() << " Gradient " << gradient << endl;
		prev_change_ = change * (pow(decision_count_, 2) + 1);				
		do_last_change();
		clear_pending_search();
		kRobustness = min<int>(1 + 3 / base_rate_, 2);
	}
	
private:

	double epsilon() const{
		if (base_rate_ > 20) return base_rate_ / 1000000;
		else if (base_rate_ > 10) return 2 * base_rate_ / 1000000;
		else return 10 * base_rate_ / 1000000;
		//return 0.00002;
		//return 1/10000.
		
		if (base_rate_ < 1) return 1;
		
		// provide fairness: the lower the rate, the stronger the step.
		return 0.5 * min<double>(0.05, base_rate_ / 100);//1/base_rate_
	}

	double avg_gradient() const {
		int base = curr_;
		double sum = 0;
		for (int i = 0; i < kRobustness; i++) {
			base += 99;
			sum += prev_gradiants_[base % 100];
		}
		return sum / kRobustness;
	}
	void guess() {
		if (first_) {
			if (start_measurement_) base_rate_ = rate();
			if (!start_measurement_) first_ = false;
		}
		if (start_measurement_) {
			next_delta = delta_for_base_rate() * base_rate_;
			setRate(base_rate_ - next_delta);
		} else {
			setRate(base_rate_ + next_delta);
		}

	}
	void adapt() {
	}
	
	virtual void init() {
		trend_count_ = 0;
		curr_ = 0;
		first_ = true;
		up_utility_ = 0;
		down_utility_ = 0;
		seq_large_incs_ = 0;
		consecutive_big_changes_ = 0;
		decision_count_ = 0;
	}

	bool first_;
	double up_utility_;
	double down_utility_;
	int seq_large_incs_;
	size_t consecutive_big_changes_;
	int trend_count_;
	int decision_count_;
	int curr_;
	double prev_gradiants_[100];
	double prev_change_;
	int kRobustness;
	double next_delta;
};

#endif
 