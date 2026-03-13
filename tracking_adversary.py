import pandas as pd
import numpy as np
from scipy.spatial import distance
import matplotlib.pyplot as plt
import os

# --- SETUP ---
script_dir = os.path.dirname(os.path.abspath(__file__))
# Change this filename to analyze different test cases
current_file = 'adversary_results.csv'
csv_path = os.path.join(script_dir, current_file) 

def run_stacked_survival_analysis(df):
    df.columns = df.columns.str.strip()
    df = df.sort_values('Time').reset_index(drop=True)
    
    # Identify Handover Failures
    first_appearances = df.groupby('myCurrentPseudonym').first().reset_index()
    first_appearances = first_appearances.sort_values('Time')
    
    real_ids = df['RealID'].unique()
    handover_fail_times = {} 
    
    # For Linkability Calculation
    total_rotations = 0
    successful_links = 0

    for _, row in first_appearances.iterrows():
        new_time = row['Time']
        new_pos = (row['X'], row['Y'])
        new_real_id = row['RealID']
        
        if new_time < df[df['RealID'] == new_real_id]['Time'].min() + 0.1:
            continue
            
        # --- LINKABILITY LOGIC ---
        prev_data_link = df[(df['Time'] < new_time) & (df['Time'] >= new_time - 11.0)]
        if not prev_data_link.empty:
            total_rotations += 1
            last_p = prev_data_link.groupby('myCurrentPseudonym').tail(1).copy()
            last_p['dist'] = last_p.apply(lambda r: distance.euclidean(new_pos, (r['X'], r['Y'])), axis=1)
            if last_p.loc[last_p['dist'].idxmin()]['RealID'] == new_real_id:
                successful_links += 1

        # --- COMPLETION LOGIC ---
        if new_real_id in handover_fail_times:
            continue

        prev_data = df[(df['Time'] < new_time) & (df['Time'] >= new_time - 11.0)]
        if prev_data.empty:
            handover_fail_times[new_real_id] = new_time
            continue
            
        last_points = prev_data.groupby('myCurrentPseudonym').tail(1).copy()
        last_points['dist'] = last_points.apply(
            lambda r: distance.euclidean(new_pos, (r['X'], r['Y'])), axis=1
        )
        best_guess = last_points.loc[last_points['dist'].idxmin()]
        
        if best_guess['RealID'] != new_real_id:
            handover_fail_times[new_real_id] = new_time

    # Map Status to Age
    vehicle_lifespans = df.groupby('RealID')['Time'].agg(['min', 'max'])
    vehicle_lifespans['lifespan'] = vehicle_lifespans['max'] - vehicle_lifespans['min']
    max_lifespan = int(vehicle_lifespans['lifespan'].max())

    total_vehicles = len(real_ids)
    active_success_rate = []
    completed_success_rate = []
    final_completed_count = 0

    for age in range(max_lifespan + 1):
        active_count = 0
        completed_count = 0
        
        for rid in real_ids:
            start_t = vehicle_lifespans.loc[rid, 'min']
            end_t = vehicle_lifespans.loc[rid, 'max']
            fail_t = handover_fail_times.get(rid, float('inf'))
            v_max_age = int(end_t - start_t)
            
            if age <= v_max_age:
                if (start_t + age) < fail_t:
                    active_count += 1
            else:
                if end_t < fail_t:
                    completed_count += 1
                    
        active_success_rate.append((active_count / total_vehicles) * 100)
        completed_success_rate.append((completed_count / total_vehicles) * 100)
        final_completed_count = completed_count # Last iteration gives total completed

    # --- SUMMARY PRINT SECTION ---
    linkability = (successful_links / total_rotations * 100) if total_rotations > 0 else 0
    completion_rate = (final_completed_count / total_vehicles) * 100

    print(f"\n" + "="*30)
    print(f"RESULTS FOR: {current_file}")
    print(f"Linkability (Adversary Accuracy): {linkability:.2f}%")
    print(f"Total Completed Tracks:           {completion_rate:.2f}%")
    print("="*30 + "\n")

    # --- PLOTTING SECTION ---
    ages = range(max_lifespan + 1)
    plt.figure(figsize=(10, 5)) 
    plt.stackplot(ages, completed_success_rate, active_success_rate, 
                  labels=['Completed Tracks (Successful)', 'Active Tracks (Successful)'],
                  colors=['#2ecc71', '#e74c3c'], alpha=0.8)
    
    # Titles for different scenarios (uncomment the one you want to use):
    plt.title("Adversary Success Rate (30s TB + 0s SP)", fontsize=20, fontweight='bold')
        #plt.title("Adversary Success Rate (30s TB + Uniform 0-1s SP)", fontsize=20, fontweight='bold')
        #plt.title("Adversary Success Rate (30s TB + Uniform 1-2s SP)", fontsize=20, fontweight='bold')
        #plt.title("Adversary Success Rate (30s TB + Uniform 2-5s SP)", fontsize=20, fontweight='bold')
        #plt.title("Adversary Success Rate (30s TB + Uniform 5-10s SP)", fontsize=20, fontweight='bold')

        #plt.title("Adversary Success Rate (500m DB + 0s SP)", fontsize=20, fontweight='bold')
        #plt.title("Adversary Success Rate (500m DB + Uniform 0-1s SP)", fontsize=20, fontweight='bold')
        #plt.title("Adversary Success Rate (500m DB + Uniform 1-2s SP)", fontsize=20, fontweight='bold')
        #plt.title("Adversary Success Rate (500m DB + Uniform 2-5s SP)", fontsize=20, fontweight='bold')
        #plt.title("Adversary Success Rate (500m DB + Uniform 5-10s SP)", fontsize=20, fontweight='bold')

    plt.xlabel("Seconds Since Vehicle Entered Network (Age)", fontsize=18)
    plt.ylabel("Population Percentage (%)", fontsize=18)
    plt.xticks(fontsize=15); plt.yticks(fontsize=15)
    plt.xlim(0, max_lifespan); plt.ylim(0, 105)
    plt.grid(True, linestyle='--', alpha=0.4)
    plt.legend(loc='upper right', fontsize=14)
    plt.tight_layout() 
    
    plt.savefig("adversary_results.pdf") 
    print("Vector graph saved as 'adversary_results.pdf'")

if __name__ == "__main__":
    if os.path.exists(csv_path):
        run_stacked_survival_analysis(pd.read_csv(csv_path))
    else:
        print(f"File {csv_path} not found.")