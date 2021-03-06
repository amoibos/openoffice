#**************************************************************
#  
#  Licensed to the Apache Software Foundation (ASF) under one
#  or more contributor license agreements.  See the NOTICE file
#  distributed with this work for additional information
#  regarding copyright ownership.  The ASF licenses this file
#  to you under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance
#  with the License.  You may obtain a copy of the License at
#  
#    http://www.apache.org/licenses/LICENSE-2.0
#  
#  Unless required by applicable law or agreed to in writing,
#  software distributed under the License is distributed on an
#  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#  KIND, either express or implied.  See the License for the
#  specific language governing permissions and limitations
#  under the License.
#  
#**************************************************************



.IF "$(WITH_CPPUNIT)" != "YES" || "$(GUI)" == "OS2"

@all:
.IF "$(GUI)" == "OS2"
	@echo "Skipping, cppunit broken."
.ELIF "$(WITH_CPPUNIT)" != "YES"
	@echo "cppunit disabled. nothing do do."
.END

.ENDIF "$(WITH_CPPUNIT)" != "YES" || "$(GUI)" == "OS2"

PRJ := ..$/..
PRJNAME := desktop
.IF "$(GUI)" == "OS2"
TARGET := qa_dpl_m
.ELSE
TARGET := qa_deployment_misc
.ENDIF

ENABLE_EXCEPTIONS := TRUE

.INCLUDE: settings.mk
.INCLUDE: $(PRJ)$/source$/deployment$/inc$/dp_misc.mk

CFLAGSCXX += $(CPPUNIT_CFLAGS)

# TODO:  On Windows, test_dp_version.cxx fails due to BOOL redefinition between
# windef.h and tools/solar.h caused by including "precompiled_desktop.hxx"; this
# hack to temporarily disable PCH will become unnecessary with the fix for issue
# 112600:
CFLAGSCXX += -DDISABLE_PCH_HACK

SHL1TARGET = $(TARGET)
SHL1OBJS = $(SLO)$/test_dp_version.obj
SHL1STDLIBS = $(CPPUNITLIB) $(DEPLOYMENTMISCLIB) $(SALLIB)
SHL1VERSIONMAP = version.map
SHL1RPATH = NONE
SHL1IMPLIB = i$(SHL1TARGET)
DEF1NAME = $(SHL1TARGET)

SLOFILES = $(SHL1OBJS)

.INCLUDE: target.mk
.INCLUDE : _cppunit.mk
