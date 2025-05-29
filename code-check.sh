#!/bin/zsh
#
# Code Quality Check
# This script runs simple checks on C++ files in the src/ directory.
#
# USAGE: ./check-cpp
#

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[CODE-CHECK]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[CODE-CHECK]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[CODE-CHECK]${NC} $1"
}

print_error() {
    echo -e "${RED}[CODE-CHECK]${NC} $1"
}

# Get all C++ files in src directory
print_status "Checking all C++ files in src/ directory..."

if [ ! -d "src" ]; then
    print_error "src/ directory not found"
    exit 1
fi

CPP_FILES=$(find src -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.c' -o -name '*.cc' -o -name '*.cxx' \))

if [ -z "$CPP_FILES" ]; then
    print_status "No C++ files found in src/ directory."
    exit 0
fi

CPP_COUNT=$(echo "$CPP_FILES" | wc -l | tr -d ' ')
print_status "Found $CPP_COUNT C++ files to check"

# Exit code tracker
EXIT_CODE=0

# Check for TODO/HACK comments in C++ files
TODO_OUTPUT=$(echo "$CPP_FILES" | tr ' ' '\n' | grep -v '^$' | xargs grep -n -i -E '(TODO|HACK)' 2>/dev/null || true)
TODO_COUNT=$(echo "$TODO_OUTPUT" | grep -v '^$' | wc -l | tr -d ' ')
if [ "$TODO_COUNT" -gt 0 ]; then
    print_warning "Found $TODO_COUNT TODO/FIXME/HACK comments in C++ files:"
    echo "$TODO_OUTPUT"
    print_warning "Consider addressing these."
fi

# Check for missing header guards or pragma once
HEADER_FILES=$(echo "$CPP_FILES" | grep -E '\.(hpp|h)$' || true)
if [ -n "$HEADER_FILES" ]; then
    while IFS= read -r file; do
        if [ -n "$file" ] && ! grep -q -E '(#pragma once|#ifndef.*_H|#ifndef.*_HPP)' "$file"; then
            print_error "Header file '$file' is missing header guard or #pragma once."
            EXIT_CODE=1
        fi
    done <<< "$HEADER_FILES"
fi

# Final result
if [ $EXIT_CODE -eq 0 ]; then
    print_success "All C++ checks passed!"
else
    print_error "C++ checks failed!"
    print_error "Fix the issues above."
fi

exit $EXIT_CODE
