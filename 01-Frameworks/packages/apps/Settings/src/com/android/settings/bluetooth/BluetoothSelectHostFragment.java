
/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.settings.bluetooth;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputFilter;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import android.provider.Settings;

import com.android.internal.app.AlertController;
import com.android.settings.R;

/**
 * Dialog fragment for selecting host.
 */
public final class BluetoothSelectHostFragment extends DialogFragment
        implements DialogInterface.OnClickListener {

	private static int selectedHost;

    public BluetoothSelectHostFragment() {

    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
		
		selectedHost = Settings.Global.getInt(getActivity().getContentResolver(),
				Settings.Global.BLUETOOTH_SELECTED_HOST, 0);

        return new AlertDialog.Builder(getActivity())
                .setTitle(R.string.bluetooth_select_host)
                .setSingleChoiceItems(R.array.bluetooth_select_host_entries, selectedHost, this)
                .setNegativeButton(android.R.string.cancel, null)
                .create();
    }

    public void onClick(DialogInterface dialog, int which) {

		if (selectedHost != which) {
			Settings.Global.putInt(getActivity().getContentResolver(),
					Settings.Global.BLUETOOTH_SELECTED_HOST, which);
			if (which == 0) {
				Log.i("BluetoothSelectHostFragment", "Faketooth Off (Phone is selected as bluetooth host)");
			} else {
				Log.i("BluetoothSelectHostFragment", "Faketooth On (PC is selected as bluetooth host)");
			}
		}
        dismiss();
    }
}
