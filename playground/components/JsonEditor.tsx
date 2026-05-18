// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

"use client";

import { useState, useEffect } from "react";
import { Check, X, RefreshCw } from "lucide-react";
import { cn, isValidJson, formatJson } from "@/lib/utils";

interface JsonEditorProps {
  value: string;
  onChange: (value: string) => void;
  placeholder?: string;
  label?: string;
  readOnly?: boolean;
}

export default function JsonEditor({
  value,
  onChange,
  placeholder = "{}",
  label,
  readOnly = false,
}: JsonEditorProps) {
  const [localValue, setLocalValue] = useState(value);
  const [isValid, setIsValid] = useState(true);
  const [isDirty, setIsDirty] = useState(false);

  useEffect(() => {
    setLocalValue(value);
    setIsDirty(false);
  }, [value]);

  const handleChange = (newValue: string) => {
    setLocalValue(newValue);
    setIsValid(isValidJson(newValue));
    setIsDirty(newValue !== value);
  };

  const handleSave = () => {
    if (isValid) {
      onChange(localValue);
      setIsDirty(false);
    }
  };

  const handleReset = () => {
    setLocalValue(value);
    setIsValid(true);
    setIsDirty(false);
  };

  const handleFormat = () => {
    const formatted = formatJson(localValue);
    setLocalValue(formatted);
    setIsDirty(formatted !== value);
  };

  return (
    <div className="flex flex-col gap-1.5">
      {label && (
        <div className="flex items-center justify-between">
          <label className="text-xs font-medium text-gray-400">{label}</label>
          <div className="flex items-center gap-1">
            {!readOnly && (
              <>
                <button
                  onClick={handleFormat}
                  title="Format JSON"
                  className="p-1 rounded text-gray-500 hover:text-gray-300 transition-colors"
                >
                  <RefreshCw className="w-3 h-3" />
                </button>
                {isDirty && (
                  <>
                    <button
                      onClick={handleReset}
                      title="Reset"
                      className="p-1 rounded text-gray-500 hover:text-red-400 transition-colors"
                    >
                      <X className="w-3 h-3" />
                    </button>
                    <button
                      onClick={handleSave}
                      disabled={!isValid}
                      title="Save"
                      className={cn(
                        "p-1 rounded transition-colors",
                        isValid
                          ? "text-gray-500 hover:text-green-400"
                          : "text-gray-600 cursor-not-allowed"
                      )}
                    >
                      <Check className="w-3 h-3" />
                    </button>
                  </>
                )}
              </>
            )}
          </div>
        </div>
      )}
      <div className="relative">
        <textarea
          value={localValue}
          onChange={(e) => handleChange(e.target.value)}
          onBlur={() => {
            if (isDirty && isValid) {
              handleSave();
            }
          }}
          placeholder={placeholder}
          readOnly={readOnly}
          spellCheck={false}
          className={cn(
            "w-full font-mono text-xs rounded border px-3 py-2 focus:outline-none resize-y min-h-[120px] leading-relaxed",
            "bg-gray-900 text-gray-300 placeholder-gray-600",
            isValid
              ? "border-gray-600 focus:border-blue-500"
              : "border-red-500/50 focus:border-red-500",
            readOnly && "opacity-75 cursor-default"
          )}
        />
        {!isValid && (
          <p className="text-xs text-red-400 mt-1">Invalid JSON</p>
        )}
      </div>
    </div>
  );
}
