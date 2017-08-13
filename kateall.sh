#!/bin/bash
kate $(find . -type f \( -name "*.h" -o -name "*.cc" \) -not -path "./build/*" -not -path "./misc/*" -not -path "./inspect/*")
