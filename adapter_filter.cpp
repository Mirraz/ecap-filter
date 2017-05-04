#include <iostream>
#include <libecap/common/autoconf.h>
#include <libecap/common/registry.h>
#include <libecap/common/errors.h>
#include <libecap/common/message.h>
#include <libecap/common/header.h>
#include <libecap/common/names.h>
#include <libecap/common/named_values.h>
#include <libecap/adapter/service.h>
#include <libecap/adapter/xaction.h>
#include <libecap/host/xaction.h>
#include "filter.h"

#define PACKAGE_NAME "eCAP Adapter Sample"
#define PACKAGE_VERSION "1.0.0"

namespace Adapter { // not required, but adds clarity

class Service: public libecap::adapter::Service {
	public:
		// About
		virtual std::string uri() const; // unique across all vendors
		virtual std::string tag() const; // changes with version and config
		virtual void describe(std::ostream &os) const; // free-format info

		// Configuration
		virtual void configure(const libecap::Options &cfg);
		virtual void reconfigure(const libecap::Options &cfg);
		void setOne(const libecap::Name &name, const libecap::Area &valArea);

		// Lifecycle
		virtual void start(); // expect makeXaction() calls
		virtual void stop(); // no more makeXaction() calls until start()
		virtual void retire(); // no more makeXaction() calls

		// Scope (XXX: this may be changed to look at the whole header)
		virtual bool wantsUrl(const char *url) const;

		// Work
		virtual MadeXactionPointer makeXaction(libecap::host::Xaction *hostx);

	private:
		filter_struct *filter;
		std::string db_uri;
		std::string default_policy;
		bool default_policy_is_allow;
};


// Calls Service::setOne() for each host-provided configuration option.
// See Service::configure().
class Cfgtor: public libecap::NamedValueVisitor {
	public:
		Cfgtor(Service &aSvc): svc(aSvc) {}
		virtual void visit(const libecap::Name &name, const libecap::Area &value) {
			svc.setOne(name, value);
		}
		Service &svc;
};


class Xaction: public libecap::adapter::Xaction {
	public:
		Xaction(libecap::host::Xaction *x, const filter_struct *f, bool d);
		virtual ~Xaction();

		// meta-information for the host transaction
		virtual const libecap::Area option(const libecap::Name &name) const;
		virtual void visitEachOption(libecap::NamedValueVisitor &visitor) const;

		// lifecycle
		virtual void start();
		virtual void stop();

		// adapted body transmission control
		virtual void abDiscard() { noBodySupport(); }
		virtual void abMake() { noBodySupport(); }
		virtual void abMakeMore() { noBodySupport(); }
		virtual void abStopMaking() { noBodySupport(); }

		// adapted body content extraction and consumption
		virtual libecap::Area abContent(libecap::size_type, libecap::size_type) { noBodySupport(); return libecap::Area(); }
		virtual void abContentShift(libecap::size_type)  { noBodySupport(); }

		// virgin body state notification
		virtual void noteVbContentDone(bool) { noBodySupport(); }
		virtual void noteVbContentAvailable() { noBodySupport(); }

	protected:
		void noBodySupport() const;

	private:
		libecap::host::Xaction *hostx; // Host transaction rep
		const filter_struct *filter;
		bool default_policy_is_allow;

		std::string getUri() const;
};

} // namespace Adapter

static const std::string CfgErrorPrefix =
	"Minimal Adapter: configuration error: ";

std::string Adapter::Service::uri() const {
	return "ecap://e-cap.org/ecap/services/sample/minimal";
}

std::string Adapter::Service::tag() const {
	return PACKAGE_VERSION;
}

void Adapter::Service::describe(std::ostream &os) const {
	os << "A minimal adapter from " << PACKAGE_NAME << " v" << PACKAGE_VERSION;
}

void Adapter::Service::configure(const libecap::Options &cfg) {
	Cfgtor cfgtor(*this);
	cfg.visitEachOption(cfgtor);

	// check for post-configuration errors and inconsistencies
	if (db_uri.empty()) throw libecap::TextException(CfgErrorPrefix + "db_uri value is not set");
	if (default_policy.empty()) throw libecap::TextException(CfgErrorPrefix + "db_uri value is not set");
}

void Adapter::Service::reconfigure(const libecap::Options &cfg) {
	db_uri.clear();
	default_policy.clear();
	configure(cfg);
}

void Adapter::Service::setOne(const libecap::Name &name, const libecap::Area &valArea) {
	const std::string value = valArea.toString();

	if (name == "db_uri") {
		if (value.empty())
			throw libecap::TextException(CfgErrorPrefix + "empty db_uri value is not allowed");
		db_uri = value;
	} else if (name == "default_policy") {
		if (value.empty())
			throw libecap::TextException(CfgErrorPrefix + "empty default_policy value is not allowed");
		if (!(value == "allow" || value == "deny"))
			throw libecap::TextException(CfgErrorPrefix + "unsupported default_policy value");
		default_policy = value;
	} else if (name.assignedHostId()) {
		// skip host-standard options we do not know or care about
	} else {
		throw libecap::TextException(CfgErrorPrefix +
				"unsupported configuration parameter: " + name.image());
	}
}

void Adapter::Service::start() {
	libecap::adapter::Service::start();
	filter = filter_construct(db_uri.c_str());
	if (filter == NULL) throw libecap::TextException("db init error");
	default_policy_is_allow = (default_policy == "allow");
}

void Adapter::Service::stop() {
	filter_destruct(filter);
	filter = NULL;
	libecap::adapter::Service::stop();
}

void Adapter::Service::retire() {
	// custom code would go here, but this service does not have one
	libecap::adapter::Service::stop();
}

bool Adapter::Service::wantsUrl(const char *url) const {
	(void)url;
	return true; // no-op is applied to all messages
}

Adapter::Service::MadeXactionPointer
Adapter::Service::makeXaction(libecap::host::Xaction *hostx) {
	return Adapter::Service::MadeXactionPointer(new Adapter::Xaction(hostx, filter, default_policy_is_allow));
}


Adapter::Xaction::Xaction(libecap::host::Xaction *x, const filter_struct *f, bool d):
		hostx(x), filter(f), default_policy_is_allow(d) {}

Adapter::Xaction::~Xaction() {
	if (libecap::host::Xaction *x = hostx) {
		hostx = 0;
		x->adaptationAborted();
	}
}

const libecap::Area Adapter::Xaction::option(const libecap::Name &) const {
	return libecap::Area(); // this transaction has no meta-information
}

void Adapter::Xaction::visitEachOption(libecap::NamedValueVisitor &) const {
	// this transaction has no meta-information to pass to the visitor
}

std::string Adapter::Xaction::getUri() const {
    typedef const libecap::RequestLine *CLRLP;
    if (CLRLP virginLine = dynamic_cast<CLRLP>(&hostx->virgin().firstLine())) {
        return virginLine->uri().toString();
    } else {
		if (CLRLP causeLine = dynamic_cast<CLRLP>(&hostx->cause().firstLine())) {
			return causeLine->uri().toString();
		} else {
			return "";
		}
    }
}

void Adapter::Xaction::start() {
	Must(hostx);
	filter_uri_result_enum result = filter_uri_is_allowed(filter, getUri().c_str());
	if (result == FILTER_URI_ALLOW || (default_policy_is_allow && result == FILTER_URI_DOESNT_EXIST)) {
		// Make this adapter non-callable
		libecap::host::Xaction *x = hostx;
		hostx = 0;
		// Tell the host to use the virgin message (request is not blacklisted)
		x->useVirgin();
	} else {
		hostx->blockVirgin(); // block access
	}
}

void Adapter::Xaction::stop() {
	hostx = 0;
	// the caller will delete
}

void Adapter::Xaction::noBodySupport() const {
	Must(!"must not be called: minimal adapter offers no body support");
	// not reached
}

// create the adapter and register with libecap to reach the host application
static const bool Registered =
	libecap::RegisterVersionedService(new Adapter::Service);
